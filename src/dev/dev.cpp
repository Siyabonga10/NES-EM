#include "dev.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <SDL3/SDL_dialog.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include "core_bindings.h"

#define MAX_MODULES    32
#define MAX_DBG_WIN    16

typedef struct { SDL_Window *w; const char *name; } DebugWindowBinding;

static DebugModule *modules[MAX_MODULES];
static int          module_count = 0;
static DebugWindowBinding dbg_wins[MAX_DBG_WIN];
static int                dbg_win_count = 0;

bool         rom_loaded   = false;
bool         debug_paused = false;
SDL_Renderer *dev_renderer = NULL;
SDL_Window   *dev_window   = NULL;

void register_debug_module(DebugModule *mod) {
    if (module_count < MAX_MODULES)
        modules[module_count++] = mod;
}

int debug_module_count(void) { return module_count; }

void register_debug_window(SDL_Window *w, const char *name) {
    if (dbg_win_count < MAX_DBG_WIN) {
        dbg_wins[dbg_win_count].w    = w;
        dbg_wins[dbg_win_count].name = name;
        dbg_win_count++;
    }
}

void unregister_debug_window(SDL_Window *w) {
    for (int i = 0; i < dbg_win_count; i++) {
        if (dbg_wins[i].w == w) {
            dbg_wins[i] = dbg_wins[--dbg_win_count];
            return;
        }
    }
}

void debug_handle_close(SDL_WindowID id) {
    for (int i = 0; i < dbg_win_count; i++) {
        if (SDL_GetWindowID(dbg_wins[i].w) == id) {
            const char *name = dbg_wins[i].name;
            for (int j = 0; j < module_count; j++) {
                if (strcmp(modules[j]->name, name) == 0) {
                    modules[j]->shutdown();
                    modules[j]->enabled = false;
                }
            }
            return;
        }
    }
}

void debug_init(SDL_Renderer *r) {
    dev_renderer = r;
    for (int i = 0; i < module_count; i++)
        modules[i]->init();
}

static char     rom_path[512]  = {0};
static bool     game_running   = false;
static bool     debug_open     = false;
static bool     settings_open  = false;
static int      max_fps        = 60;
static int      scaling_mode   = 0;

static void SDLCALL file_dialog_callback(void *userdata, const char *const *filelist, int filter) {
    if (filelist && filelist[0])
        strncpy(rom_path, filelist[0], sizeof(rom_path) - 1);
}

int get_scaling_mode(void) { return scaling_mode; }
int get_max_fps(void)       { return max_fps; }

static Cartriadge     *cart  = NULL;
static SDL_AudioStream *audio_stream = NULL;
static float           audio_volume = 0.3f;

static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    (void)userdata;
    (void)total_amount;
    int needed = additional_amount / (int)sizeof(float);
    int pos    = 0;
    while (pos < needed) {
        int chunk = needed - pos;
        if (chunk > 4096) chunk = 4096;
        static float buf[4096];
        apu_mix_samples(buf, chunk);
        if (audio_volume != 1.0f) {
            for (int i = 0; i < chunk; i++) buf[i] *= audio_volume;
        }
        SDL_PutAudioStreamData(stream, buf, chunk * (int)sizeof(float));
        pos += chunk;
    }
}

static void start_rom(const char *path) {
    if (game_running) {
        shutdown_cpu();
        kill_ppu();
        free(cart->chr_ram);
        free(cart->prg_ram);
        free(cart->pg_rom);
        free(cart->ch_rom);
        free(cart);
    }

    cart = (Cartriadge *)malloc(sizeof(Cartriadge));
    memset(cart, 0, sizeof(Cartriadge));
    load_cartridge((char *)path, cart);
    connect_cartridge_to_bus(cart);
    connect_controller_to_console();
    boot_nes_audio();

    SDL_AudioSpec spec = {SDL_AUDIO_F32, 1, 44100};
    if (audio_stream) SDL_DestroyAudioStream(audio_stream);
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, NULL);
    if (audio_stream) SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audio_stream));

    boot_ppu();
    boot_cpu();
    rom_loaded   = true;
    game_running = true;
}

static void stop_rom(void) {
    game_running = false;
    rom_loaded   = false;
    shutdown_cpu();
    kill_ppu();
    if (cart) {
        free(cart->chr_ram);
        free(cart->prg_ram);
        free(cart->pg_rom);
        free(cart->ch_rom);
        free(cart);
        cart = NULL;
    }
    if (audio_stream) {
        SDL_DestroyAudioStream(audio_stream);
        audio_stream = NULL;
    }
}

void debug_menu(void) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load ROM...")) {
                memset(rom_path, 0, sizeof(rom_path));
                debug_open = true;
            }
            if (game_running && ImGui::MenuItem("Close ROM")) stop_rom();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                SDL_Event ev;
                ev.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&ev);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            for (int i = 0; i < module_count; i++) {
                DebugModule *m = modules[i];
                if (m->requires_rom && !rom_loaded) {
                    ImGui::BeginDisabled();
                    ImGui::MenuItem(m->name);
                    ImGui::EndDisabled();
                } else {
                    ImGui::MenuItem(m->name, NULL, &m->enabled);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Settings")) {
            ImGui::MenuItem("Settings...", NULL, &settings_open);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (debug_open) {
        ImGui::SetNextWindowSize(ImVec2(450, 120), ImGuiCond_Appearing);
        ImGui::Begin("Load ROM", &debug_open);
        ImGui::InputText("Path", rom_path, sizeof(rom_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            SDL_DialogFileFilter filters[] = {{"NES ROMs", "nes"}};
            SDL_ShowOpenFileDialog(file_dialog_callback, NULL, dev_window, filters, 1, NULL, false);
        }
        if (ImGui::Button("Load") && rom_path[0]) start_rom(rom_path);
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) debug_open = false;
        ImGui::End();
    }

    if (settings_open) {
        ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Appearing);
        ImGui::Begin("Settings", &settings_open);
        ImGui::SliderInt("Max FPS", &max_fps, 0, 480, max_fps == 0 ? "Unlimited" : "%d");
        const char *modes[] = {"Nearest", "Linear"};
        int cur = scaling_mode;
        ImGui::Combo("Scaling Mode", &cur, modes, 2);
        if (cur != scaling_mode) scaling_mode = cur;
        ImGui::End();
    }
}

void debug_update(void) {
    for (int i = 0; i < module_count; i++) {
        if (modules[i]->enabled)
            modules[i]->update();
    }
}

void debug_render(void) {
    for (int i = 0; i < module_count; i++) {
        if (modules[i]->enabled)
            modules[i]->render();
    }
}

void debug_shutdown(void) {
    stop_rom();
    for (int i = 0; i < module_count; i++)
        modules[i]->shutdown();
}

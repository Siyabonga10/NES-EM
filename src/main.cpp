#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include "dev/dev.h"

#define BASE_WIDTH  256
#define BASE_HEIGHT 240
#define VISIBLE_H   224

int main(int argc, char **argv) {
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();

    SDL_Window   *window   = SDL_CreateWindow("NES Emulator", 1024, 896, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 0);

    SDL_Texture *game_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                                   SDL_TEXTUREACCESS_STREAMING, BASE_WIDTH, BASE_HEIGHT);
    SDL_SetTextureScaleMode(game_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(game_texture, SDL_BLENDMODE_NONE);
    set_game_texture(game_texture);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 16.0f);

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    dev_window   = window;
    dev_renderer = renderer;
    debug_init(renderer);

    if (argc > 1) {
        start_rom(argv[1]);
    }

    Uint64 frame_start = SDL_GetTicks();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                debug_handle_close(event.window.windowID);
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        debug_menu();
        debug_update();
        debug_render();

        if (rom_loaded) {
            ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetFrameHeight() + 5));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5f));
            ImGui::Begin("##fps", NULL,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("FPS: %.0f", get_game_fps());
            ImGui::End();
            ImGui::PopStyleColor();
        }

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Texture *gt = get_game_texture();
        if (rom_loaded && gt) {
            int ww, wh;
            SDL_GetWindowSize(window, &ww, &wh);
            float mh = ImGui::GetFrameHeight();
            float ah = (float)wh - mh;
            float aw = (float)ww;
            float s = aw / BASE_WIDTH;
            if (ah / VISIBLE_H < s) s = ah / VISIBLE_H;
            float gw = BASE_WIDTH * s, gh = VISIBLE_H * s;
            SDL_FRect src = {0, 8, BASE_WIDTH, VISIBLE_H};
            SDL_FRect dst = {(aw - gw) * 0.5f, mh + (ah - gh) * 0.5f, gw, gh};
            SDL_RenderTexture(renderer, gt, &src, &dst);
        }

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        int mfps = get_max_fps();
        if (mfps > 0) {
            int elapsed = (int)(SDL_GetTicks() - frame_start);
            int delay   = 1000 / mfps - elapsed;
            if (delay > 0) SDL_Delay(delay);
        }
        frame_start = SDL_GetTicks();
    }

    debug_shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyTexture(game_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

#include "dev.h"
#include <imgui.h>
#include "core_bindings.h"

#define BASE_WIDTH  256
#define BASE_HEIGHT 240
#define VISIBLE_H   224

static SDL_Texture *game_tex = NULL;
static float        fps_counter = 0;
static Uint64       last_fps    = 0;
static int          fps_frames  = 0;

static void init(void) {}

void set_game_texture(SDL_Texture *tex) { game_tex = tex; }
SDL_Texture *get_game_texture(void) { return game_tex; }
float get_game_fps(void) { return fps_counter; }

static void update(void) {
    if (!rom_loaded || !game_tex) return;
    if (debug_paused) return;

    const bool *keys = SDL_GetKeyboardState(NULL);

    FrameData *frame = NULL;
    Uint64     started = SDL_GetTicks();

    do {
        ControllerKeyStates cks = {
            .a_pressed      = keys[SDL_SCANCODE_A],
            .b_pressed      = keys[SDL_SCANCODE_B],
            .up_pressed     = keys[SDL_SCANCODE_UP],
            .down_pressed   = keys[SDL_SCANCODE_DOWN],
            .left_pressed   = keys[SDL_SCANCODE_LEFT],
            .right_pressed  = keys[SDL_SCANCODE_RIGHT],
            .start_pressed  = keys[SDL_SCANCODE_RETURN],
            .select_pressed = keys[SDL_SCANCODE_SPACE]};
        frame = tick_cpu_once(&cks);
        if (get_bp_addr() != 0 && get_pc() == get_bp_addr()) {
            debug_paused = true;
            break;
        }
        if (get_cycle_break_on() && get_elapsed_clock_cycles() >= get_cycle_target()) {
            debug_paused = true;
            break;
        }
    } while (!frame->is_new_frame && (SDL_GetTicks() - started) < 16);

    if (frame->is_new_frame) {
        SDL_UpdateTexture(game_tex, NULL, frame->data, BASE_WIDTH * (int)sizeof(NesColor));
        frame->is_new_frame = false;
    }
    update_apu();
}

static void render(void) {
    if (!rom_loaded) return;

    fps_frames++;
    Uint64 now = SDL_GetTicks();
    if (now - last_fps >= 1000) {
        fps_counter = (float)fps_frames * 1000.0f / (float)(now - last_fps);
        fps_frames  = 0;
        last_fps    = now;
    }
}

static void shutdown(void) {}

static DebugModule _mod = {
    "Game Window", init, update, render, shutdown, false, true
};
REGISTER_DEBUG_MODULE(_mod)

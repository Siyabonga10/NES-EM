#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "core/controller.h"
#include "core/cpu.h"
#include "core/bus.h"
#include "core/instructions.h"
#include "core/addressing_modes.h"
#include "core/rom_loader.h"
#include "core/ppu.h"
#include "core/audio.h"
#include "core/frameData.h"

#define BASE_WIDTH 256
#define BASE_HEIGHT 240
#define SCALING_FACTOR 4

static SDL_AudioStream *audio_stream;
static float            audio_volume = 0.3f;
static SDL_Texture     *game_texture;
static int              target_fps = 60;
static SDL_Renderer    *renderer;
static SDL_Window      *window;

static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
  (void)userdata;
  (void)total_amount;
  int needed = additional_amount / (int)sizeof(float);
  int pos    = 0;
  while (pos < needed) {
    int chunk = needed - pos;
    if (chunk > 4096)
      chunk = 4096;
    static float buf[4096];
    apu_mix_samples(buf, chunk);
    if (audio_volume != 1.0f) {
      for (int i = 0; i < chunk; i++)
        buf[i] *= audio_volume;
    }
    SDL_PutAudioStreamData(stream, buf, chunk * (int)sizeof(float));
    pos += chunk;
  }
}

void draw_frame(void) {
  SDL_FRect src = {0, 8, BASE_WIDTH, BASE_HEIGHT - 16};
  SDL_FRect dst = {0, 0, BASE_WIDTH * SCALING_FACTOR, (BASE_HEIGHT - 16) * SCALING_FACTOR};
  SDL_RenderTexture(renderer, game_texture, &src, &dst);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Please provide a rom file");
    return 1;
  }

  SDL_SetMainReady();
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  TTF_Init();

  window   = SDL_CreateWindow("NES Emulator",
                              BASE_WIDTH * SCALING_FACTOR,
                              (BASE_HEIGHT - 16) * SCALING_FACTOR,
                              0);
  renderer = SDL_CreateRenderer(window, "software");
  SDL_SetRenderVSync(renderer, 0);

  game_texture = SDL_CreateTexture(renderer,
                                   SDL_PIXELFORMAT_ABGR8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   BASE_WIDTH, BASE_HEIGHT);

  SDL_SetTextureBlendMode(game_texture, SDL_BLENDMODE_NONE);
  Cartriadge *test_cartridge = malloc(sizeof(Cartriadge));
  load_cartridge(argv[1], test_cartridge);
  connect_cartridge_to_bus(test_cartridge);
  connect_controller_to_console();
  boot_nes_audio();

  SDL_AudioSpec spec = {SDL_AUDIO_F32, 1, 44100};
  audio_stream       = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, NULL);
  if (audio_stream) {
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audio_stream));
  }

  boot_ppu();
  boot_cpu();

  bool prev_shift  = false;
  bool prev_equals = false;
  bool prev_minus  = false;
  bool prev_d      = false;
  bool prev_s      = false;

  int    frame_start_pc = -1;
  Uint64 frame_start    = SDL_GetTicks();
  bool   running        = true;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT)
        running = false;
    }

    const bool *keys = SDL_GetKeyboardState(NULL);

    prev_equals = keys[SDL_SCANCODE_EQUALS];
    prev_minus  = keys[SDL_SCANCODE_MINUS];
    prev_d      = keys[SDL_SCANCODE_D];
    prev_s      = keys[SDL_SCANCODE_S];

    FrameData *frame = tick_cpu_once(&(ControllerKeyStates){
        .a_pressed      = keys[SDL_SCANCODE_A],
        .b_pressed      = keys[SDL_SCANCODE_B],
        .up_pressed     = keys[SDL_SCANCODE_UP],
        .down_pressed   = keys[SDL_SCANCODE_DOWN],
        .left_pressed   = keys[SDL_SCANCODE_LEFT],
        .right_pressed  = keys[SDL_SCANCODE_RIGHT],
        .start_pressed  = keys[SDL_SCANCODE_RETURN],
        .select_pressed = keys[SDL_SCANCODE_SPACE]});

    int  current_pc    = get_pc();
    bool should_render = frame->is_new_frame;

    if (should_render) {
      frame_start_pc = current_pc;
      if (frame->is_new_frame)
        SDL_UpdateTexture(game_texture, NULL, frame->data, BASE_WIDTH * (int)sizeof(NesColor));
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);
      draw_frame();
      SDL_RenderPresent(renderer);
    }
    update_apu();
  }

  TTF_Quit();
  SDL_DestroyTexture(game_texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  free(test_cartridge->chr_ram);
  free(test_cartridge->prg_ram);
  free(test_cartridge->pg_rom);
  free(test_cartridge->ch_rom);
  shutdown_cpu();
  kill_ppu();
  free(test_cartridge);
}

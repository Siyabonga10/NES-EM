#include <stdio.h>
#include "core/controller.h"
#include "core/cpu.h"
#include "core/bus.h"
#include "core/rom_loader.h"
#include "core/ppu.h"
#include "core/audio.h"
#include <raylib.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>

#define BASE_WIDTH 256
#define BASE_HEIGHT 240
#define SCALING_FACTOR 4

static AudioStream master_stream;
static float       audio_volume = 0.3f;
static Texture2D   game_texture;
static int         target_fps = 60;

static void raylib_audio_callback(void *buffer, unsigned int frames) {
  apu_mix_samples((float *)buffer, frames);
}

static char *test_files[] = {
    "test-roms/01-implied.nes",
    "test-roms/02-immediate.nes",
    "test-roms/03-zero_page.nes",
    "test-roms/04-zp_xy.nes",
    "test-roms/05-absolute.nes",
    "test-roms/06-abs_xy.nes",
    "test-roms/07-ind_x.nes",
    "test-roms/08-ind_y.nes",
    "test-roms/09-branches.nes",
    "test-roms/10-stack.nes",
    "test-roms/11-jmp_jsr.nes",
    "test-roms/12-rts.nes",
    "test-roms/13-rti.nes",
    "test-roms/14-brk.nes",
    "test-roms/15-special.nes"};

void draw_frame(FrameData data) {
  if (!data.is_new_frame)
    return;

  UpdateTexture(game_texture, data.data);

  BeginDrawing();
  ClearBackground(BLACK);
  DrawTexturePro(game_texture,
                 (Rectangle){0, 8, BASE_WIDTH, BASE_HEIGHT - 16},
                 (Rectangle){0, 0, BASE_WIDTH * SCALING_FACTOR, (BASE_HEIGHT - 16) * SCALING_FACTOR},
                 (Vector2){0, 0}, 0.0f, WHITE);
  DrawFPS(10, 10);
  EndDrawing();
}

void process_input() {
  if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) {
    target_fps += 30;
    if (target_fps > 240)
      target_fps = 60;
    SetTargetFPS(target_fps);
  }
  if (IsKeyPressed(KEY_EQUAL)) {
    audio_volume += 0.05f;
    if (audio_volume > 1.0f)
      audio_volume = 1.0f;
    SetMasterVolume(audio_volume);
  }
  if (IsKeyPressed(KEY_MINUS)) {
    audio_volume -= 0.05f;
    if (audio_volume < 0.0f)
      audio_volume = 0.0f;
    SetMasterVolume(audio_volume);
  }
}

static bool in_debug     = false;
static bool in_slow_mode = false;

void process_debug_input() {
  if (IsKeyPressed(KEY_D)) {
    in_debug = !in_debug;
    SetWindowSize(in_debug ? BASE_WIDTH * SCALING_FACTOR * 1.5 : BASE_WIDTH * SCALING_FACTOR, GetScreenHeight());
  }

  if (IsKeyPressed(KEY_S)) {
    in_slow_mode = !in_slow_mode;
    SetTargetFPS(in_slow_mode ? 5 : 60);
  }
}

void draw_debug() {
  int dbg_offset = BASE_WIDTH * SCALING_FACTOR;
  int padding    = 10;
  int font_size  = 30;

  if (!in_debug)
    return;
  DrawLine(dbg_offset, 0, dbg_offset, GetScreenHeight(), WHITE);

  int staus = read_byte(get_cpu_status_register());
  int PC    = get_pc();
  int A     = read_byte(get_cpu_accumulator());
  int X     = read_byte(get_cpu_x_register());
  int Y     = read_byte(get_cpu_y_register());

  DrawText(TextFormat("PC %X", PC), dbg_offset + padding, padding, font_size, WHITE);
  DrawText(TextFormat("A  %X", A), dbg_offset + padding, 2 * padding + font_size, font_size, WHITE);
  DrawText(TextFormat("X  %X", X), dbg_offset + padding, 3 * padding + 2 * font_size, font_size, WHITE);
  DrawText(TextFormat("Y  %X", Y), dbg_offset + padding, 4 * padding + 3 * font_size, font_size, WHITE);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Please provide a rom file");
    return 1;
  }
  Cartriadge *test_cartridge = malloc(sizeof(Cartriadge));

  InitWindow(BASE_WIDTH * SCALING_FACTOR, (BASE_HEIGHT - 16) * SCALING_FACTOR, "testing");
  InitAudioDevice();

  Image img    = GenImageColor(BASE_WIDTH, BASE_HEIGHT, BLACK);
  game_texture = LoadTextureFromImage(img);
  UnloadImage(img);

  load_cartridge(argv[1], test_cartridge);
  connect_cartridge_to_bus(test_cartridge);
  connect_controller_to_console();
  boot_nes_audio();

  master_stream = LoadAudioStream(44100, 32, 1);
  SetAudioStreamCallback(master_stream, raylib_audio_callback);
  SetMasterVolume(audio_volume);
  PlayAudioStream(master_stream);

  boot_ppu();
  boot_cpu();
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    process_input();
    process_debug_input();
    FrameData *frame = tick_cpu(&(ControllerKeyStates){
        .a_pressed      = IsKeyDown(KEY_A),
        .b_pressed      = IsKeyDown(KEY_B),
        .up_pressed     = IsKeyDown(KEY_UP),
        .down_pressed   = IsKeyDown(KEY_DOWN),
        .left_pressed   = IsKeyDown(KEY_LEFT),
        .right_pressed  = IsKeyDown(KEY_RIGHT),
        .start_pressed  = IsKeyDown(KEY_ENTER),
        .select_pressed = IsKeyDown(KEY_SPACE)});
    draw_frame(*frame);
    draw_debug();
    update_apu();
  }

  UnloadTexture(game_texture);
  CloseAudioDevice();
  free(test_cartridge->chr_ram);
  free(test_cartridge->prg_ram);
  free(test_cartridge->pg_rom);
  free(test_cartridge->ch_rom);
  shutdown_cpu();
  kill_ppu();
  free(test_cartridge);
}

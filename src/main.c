#include <stdio.h>
#include "core/controller.h"
#include "core/cpu.h"
#include "core/bus.h"
#include "core/cartriadge.h"
#include "core/ppu.h"
#include "core/audio.h"
#include <raylib.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>

#define BASE_WIDTH 256
#define BASE_HEIGHT 240
#define SCALING_FACTOR 4

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

void draw_frame(FrameData data)
{
    if (!data.is_new_frame)
        return;
    BeginDrawing();
    ClearBackground(BLACK);
    int clip_top = 8;
    int clip_bot = 8;
    for (int i = clip_top; i < BASE_HEIGHT - clip_bot; i++)
    {
        for (int j = 0; j < BASE_WIDTH; j++)
        {
            DrawRectangle(j * SCALING_FACTOR, (i - clip_top) * SCALING_FACTOR, SCALING_FACTOR, SCALING_FACTOR, *(Color *)(data.data + i * BASE_WIDTH + j));
        }
    }
    DrawFPS(10, 10);
    EndDrawing();
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Please provide a rom file");
        return 1;
    }
    Cartriadge *test_cartridge = malloc(sizeof(Cartriadge));
    InitWindow(BASE_WIDTH * SCALING_FACTOR, (BASE_HEIGHT - 16) * SCALING_FACTOR, "testing");
    InitAudioDevice();
    load_cartridge(argv[1], test_cartridge);
    connect_cartridge_to_bus(test_cartridge);
    connect_controller_to_console();
    boot_nes_audio();
    SetTargetFPS(60);
    boot_ppu();
    boot_cpu();
    while (!WindowShouldClose())
    {
        FrameData *frame = tick_cpu(&(ControllerKeyStates){
            .a_pressed = IsKeyDown(KEY_A),
            .b_pressed = IsKeyDown(KEY_B),
            .up_pressed = IsKeyDown(KEY_UP),
            .down_pressed = IsKeyDown(KEY_DOWN),
            .left_pressed = IsKeyDown(KEY_LEFT),
            .right_pressed = IsKeyDown(KEY_RIGHT),
            .start_pressed = IsKeyDown(KEY_ENTER),
            .select_pressed = IsKeyDown(KEY_SPACE)});
        draw_frame(*frame);
        update_apu();
    }
    CloseAudioDevice();
    free(test_cartridge->chr_ram);
    free(test_cartridge->pg_rom);
    free(test_cartridge->ch_rom);
    shutdown_cpu();
    kill_ppu();
    free(test_cartridge);
}

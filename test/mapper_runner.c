#include "core/cpu.h"
#include "core/bus.h"
#include "core/rom_loader.h"
#include "core/ppu.h"
#include "core/audio.h"
#include "core/controller.h"
#include "core/frameData.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FW 256
#define FH 224
#define CLIP 8

static Texture2D game_tex;
static int rom_index = 0;
static Cartriadge cart;
static bool running = true;
static bool unsupported;
static int rom_count = 0;

static char *test_roms[] = {
    "test-roms/mappers/M0_P32K_C8K_V.nes",
    "test-roms/mappers/M1_P128K.nes",
    "test-roms/mappers/M2_P128K_V.nes",
    "test-roms/mappers/M3_P32K_C32K_H.nes",
    "test-roms/mappers/M4_P128K.nes",
    "test-roms/mappers/M7_P128K.nes",
    "test-roms/mappers/M7_P128K.nes",
    "test-roms/mappers/M66_P64K_C16K_V.nes",
    "test-roms/mappers/M69_P128K_C64K_S8K.nes",
    "test-roms/mappers/M34_P128K_H.nes",
};

static void cleanup_cartridge(void) {
    free(cart.pg_rom);
    free(cart.ch_rom);
    free(cart.chr_ram);
    free(cart.prg_ram);
    memset(&cart, 0, sizeof(cart));
}

static void load_current_rom(void) {
    unsupported = false;
    memset(&cart, 0, sizeof(cart));
    load_cartridge(test_roms[rom_index], &cart);
}

static void draw_frame(FrameData data) {
    if (!data.is_new_frame) return;
    UpdateTexture(game_tex, data.data + CLIP * FW);

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(game_tex,
        (Rectangle){0, 0, FW, FH},
        (Rectangle){0, 0, FW * 3, FH * 3},
        (Vector2){0, 0}, 0.0f, WHITE);

    DrawText(test_roms[rom_index], 8, 4, 12, GREEN);
    DrawText(TextFormat("%d/%d", rom_index + 1, rom_count), 8, 20, 12, GRAY);
    DrawText("SPACE=next  ESC=quit", 8, GetScreenHeight() - 20, 12, GRAY);
    EndDrawing();
}

int main(void) {
    rom_count = sizeof(test_roms) / sizeof(test_roms[0]);

    InitWindow(FW * 3, FH * 3, "Mapper Test Runner");
    SetTargetFPS(60);

    Image img = GenImageColor(FW, FH, BLACK);
    game_tex = LoadTextureFromImage(img);
    UnloadImage(img);

    connect_controller_to_console();
    boot_nes_audio();
    load_current_rom();
    boot_ppu();
    boot_cpu();

    while (running && !WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            shutdown_cpu();
            kill_ppu();
            cleanup_cartridge();

            rom_index++;
            if (rom_index >= rom_count) rom_index = 0;

            boot_ppu();
            load_current_rom();
            boot_cpu();
        }

        if (IsKeyPressed(KEY_ESCAPE)) running = false;

        FrameData *frame = tick_cpu(&(ControllerKeyStates){0});
        draw_frame(*frame);
        update_apu();
    }

    shutdown_cpu();
    kill_ppu();
    cleanup_cartridge();
    UnloadTexture(game_tex);
    CloseWindow();
    return 0;
}

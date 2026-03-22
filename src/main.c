#include <stdio.h>
#include "core/controller.h"
#include "core/cpu.h"
#include "core/bus.h"
#include "core/cartriadge.h"
#include "core/ppu.h"
#include <raylib.h>

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

void drawFrame(FrameData data)
{
    if (!data.is_new_frame)
        return;
    BeginDrawing();
    ClearBackground(BLACK);
    for (int i = 0; i < BASE_HEIGHT; i++)
    {
        for (int j = 0; j < BASE_WIDTH; j++)
        {
            DrawRectangle(j * SCALING_FACTOR, i * SCALING_FACTOR, SCALING_FACTOR, SCALING_FACTOR, *(Color *)(data.data + i * BASE_WIDTH + j));
        }
    }
    DrawFPS(10, 10);
    EndDrawing();
}

int main()
{
    Cartriadge *testCartriadge = malloc(sizeof(Cartriadge));
    loadCartriadge("test-roms/tetris2.nes", testCartriadge);
    connectCartriadgeToBus(testCartriadge);
    connectControllerToConsole();
    InitWindow(BASE_WIDTH * SCALING_FACTOR, BASE_HEIGHT * SCALING_FACTOR, "testing");
    bootPPU();
    bootCPU();
    while (!WindowShouldClose())
    {
        tickCPU(&(ControllerKeyStates){
            .a_pressed = IsKeyDown(KEY_A),
            .b_pressed = IsKeyDown(KEY_B),
            .up_pressed = IsKeyDown(KEY_UP),
            .down_pressed = IsKeyDown(KEY_DOWN),
            .left_pressed = IsKeyDown(KEY_LEFT),
            .right_pressed = IsKeyDown(KEY_RIGHT),
            .start_pressed = IsKeyDown(KEY_ENTER),
            .select_pressed = IsKeyDown(KEY_SPACE)});
        drawFrame(requestFrame());
    }
    free(testCartriadge->mem);
    shutdownCPU();
    killPPU();
    free(testCartriadge);
}
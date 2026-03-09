#include "ppu.h"
#include "bus.h"
#include <assert.h>
#include <stdint.h>
#include <raylib.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#define INTERNAL_REGISTER_SIZE 4
#define EXPOSED_REGISTERS_SIZE 9
#define DOTS_PER_CYCLE 341
#define CYCLES_PER_FRAME 262
#define VISIBLE_DOTS 256
#define VISIBLE_SCAN_LINES 240
#define BYTES_PER_PIXEL 3
#define TILES_PER_ROW 32
#define TILES_PER_COLUM 30
#define TILE_SIZE 8
#define PALETTE_RAM_SIZE 32
#define BYTES_PER_TILE 16
#define BASE_WIDTH 256
#define BASE_HEIGHT 240
#define SCALLING_FACTOR 4.0f
#define W_RAM_SIZE 0x800

static int current_dot = 0;
static int current_row = 0;
static int cycle_count = 0;

enum InternalReg
{
    Internal_V = 0,
    Internal_T,
    Internal_X,
    Internal_W
};

static int16_t registers[EXPOSED_REGISTERS_SIZE] = {0};
static unsigned char vram[W_RAM_SIZE] = {0};
static unsigned char palette_ram[PALETTE_RAM_SIZE] = {0};
static int16_t internal_registers[INTERNAL_REGISTER_SIZE] = {0};
static unsigned char read_buffer = {0};

void drawDBGScreen();
void drawTileDBG(int row, int col, unsigned char nametable_byte);
static void renderFrame();

int ppu_to_vram(int ppu_address)
{
    if (ppu_address >= 0x2000 && ppu_address <= 0x3EFF)
    {
        ppu_address -= 0x2000;
        ppu_address %= W_RAM_SIZE;
        return ppu_address;
    }
    return 0;
}

unsigned char readPPU(int addr)
{
    int register_index = addr - 0x2000;
    assert(addr >= 0x2000 && addr < 0x4000);
    addr = (register_index % 8) + 0x2000;
    register_index %= 8;
    switch (addr)
    {
    case 0x2000:
        return registers[0];
    case 0x2002:
        internal_registers[Internal_W] = 0;

        unsigned char status_reg = (unsigned char)registers[2];
        registers[2] &= 0b01111111;
        return status_reg;
    case 0x2004:
        return (unsigned char)registers[register_index];
    case 0x2007:
        unsigned char current_read_buff = read_buffer;
        read_buffer = vram[ppu_to_vram(internal_registers[Internal_V])];
        if ((registers[0] & 0x4) == 0)
            internal_registers[Internal_V]++;
        else
            internal_registers[Internal_V] += 32;
        return current_read_buff;
    }
}

void writePPU(int addr, unsigned char byte)
{
    int register_index = addr - 0x2000;
    assert(addr >= 0x2000 && addr < 0x4000);
    addr = (register_index % 8) + 0x2000;
    register_index %= 8;
    switch (addr)
    {
    case 0x2007:
        registers[register_index] = byte;
        int address = internal_registers[Internal_V];
        if (address >= 0x3F00)
            palette_ram[address % PALETTE_RAM_SIZE] = byte;
        else
            vram[ppu_to_vram(address)] = byte;
        if ((registers[0] & 0x4) == 0)
            internal_registers[Internal_V]++;
        else
            internal_registers[Internal_V] += 32;
        break;
    case 0x2000:
        bool old_nmi_output = (registers[0] & 0x80) != 0;
        registers[0] = byte;
        bool new_nmi_output = (registers[0] & 0x80) != 0;

        if (!old_nmi_output && new_nmi_output && (registers[2] & 0x80) != 0)
            triggerNMI();
    case 0x2001:
    case 0x2003:
    case 0x2004:
        registers[register_index] = byte;
        break;
    case 0x2006:
        if (internal_registers[Internal_W] == 0)
        {
            registers[register_index] &= 0x00FF;
            registers[register_index] |= ((int)byte << 8);
            internal_registers[Internal_T] &= 0x00FF;
            internal_registers[Internal_T] |= ((int)byte << 8);
            internal_registers[Internal_W] = 1;
        }
        else
        {
            registers[register_index] &= 0xFF00;
            registers[register_index] |= byte;
            internal_registers[Internal_T] &= 0xFF00;
            internal_registers[Internal_T] |= byte;
            internal_registers[Internal_V] = internal_registers[Internal_T];
            internal_registers[Internal_W] = 0;
        }
        break;
    case 0x4014:
        registers[8] = byte;
        break;
    }
}

void tick()
{

    cycle_count++;
    current_dot++;
    if (current_dot >= DOTS_PER_CYCLE)
    {
        current_dot = 0;
        current_row++;
        if (current_row >= CYCLES_PER_FRAME)
        {
            current_row = 0;
            renderFrame();
        }
    }

    if (current_row == 241 && current_dot == 1)
    {
        registers[2] |= 0x80;
        if ((registers[0] & 0x80) != 0)
            triggerNMI();
    }

    if (current_row == 261 && current_dot == 1)
        registers[2] &= 0b01111111;
}

void bootPPU()
{
    connect_ppu_to_bus(tick, readPPU, writePPU);
    InitWindow(BASE_WIDTH * SCALLING_FACTOR, BASE_HEIGHT * SCALLING_FACTOR, "NES emulator");
    SetTargetFPS(60);
}

static void renderFrame()
{
    BeginDrawing();
    ClearBackground(BLACK);
    const char *time_text = TextFormat("NES Emulator: %.2f FPS", roundf(1.0f / GetFrameTime()));
    SetWindowTitle(time_text);
    drawDBGScreen();
    EndDrawing();
}

void drawDBGScreen()
{
    for (int row = 0; row < TILES_PER_COLUM; row++)
    {
        for (int col = 0; col < TILES_PER_ROW; col++)
        {
            unsigned char nametable_byte = vram[row * TILES_PER_ROW + col];
            drawTileDBG(row, col, nametable_byte);
        }
    }
}

Color getTileColor(int indx)
{
    switch (indx)
    {
    case 0:
        return BLACK;
    case 1:
        return WHITE;
    case 2:
        return WHITE;
    case 3:
        return WHITE;
    default:
        return PINK;
    }
}

void drawTileDBG(int row, int col, unsigned char nametable_byte)
{
    for (int i = 0; i < TILE_SIZE; i++)
    {
        int offset = registers[0] & 16 == 0 ? 0 : 0x1000;
        unsigned char low = readBytePPU(offset + BYTES_PER_TILE * nametable_byte + i);
        unsigned char high = readBytePPU(offset + BYTES_PER_TILE * nametable_byte + TILE_SIZE + i);

        for (int j = 0; j < TILE_SIZE; j++)
        {
            int shiftVal = (TILE_SIZE - 1 - j);
            int mask = 1 << shiftVal;
            int val = ((low & mask) >> shiftVal) | ((high & mask) >> shiftVal);
            DrawRectangle(
                col * TILE_SIZE * SCALLING_FACTOR + j * SCALLING_FACTOR,
                row * TILE_SIZE * SCALLING_FACTOR + i * SCALLING_FACTOR,
                SCALLING_FACTOR,
                SCALLING_FACTOR,
                getTileColor(val));
        }
    }

    // if (nametable_byte != 36)
    //     DrawText(TextFormat("%i", nametable_byte), col * TILE_SIZE * SCALLING_FACTOR, row * TILE_SIZE * SCALLING_FACTOR, 20, WHITE);
}

/*
CPU instruction tests are probably passing, so the random patterns result from us fetching what is at tile 0
*/
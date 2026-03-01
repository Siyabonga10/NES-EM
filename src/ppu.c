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
#define DOTS_PER_CYCLE 340
#define CYCLES_PER_FRAME 261
#define VISIBLE_DOTS 256
#define VISIBLE_SCAN_LINES 240
#define BYTES_PER_PIXEL 3
#define TILES_PER_ROW 32
#define TILES_PER_COLUM 30
#define TILE_SIZE 8
#define PALETTE_RAM_SIZE 32
#define BYTES_PER_TILE 16

#define W_RAM_SIZE 0x800

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

int vram_to_ppu(int vram_address)
{
    vram_address &= 0x07FF;
    return 0x2000 + vram_address;
}

static void renderFrame();
unsigned char readPPU(int addr)
{
    int register_index = addr - 0x2000;
    assert(addr >= 0x2000 && addr < 0x4000);
    addr = (register_index % 8) + 0x2000;
    register_index %= 8;
    switch (addr)
    {
    case 0x2002:
        internal_registers[Internal_W] = 0;
        unsigned char status_reg = (unsigned char)registers[register_index];
        registers[2] &= 0b01111111;
        return status_reg;
    case 0x2004:
        return (unsigned char)registers[register_index];
        break;
    case 0x2007:
        unsigned char current_read_buff = read_buffer;
        read_buffer = vram[ppu_to_vram(internal_registers[Internal_V])];
        if ((registers[0] & 0x4) == 0)
            internal_registers[Internal_V]++;
        else
            internal_registers[Internal_V] += 32;
        return current_read_buff;
        break;
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
    case 0x2001:
    case 0x2003:
    case 0x2004:
        registers[register_index] = byte;
        break;
    // case 0x2005:
    //     // TODO: Fix scrolling
    //     break;
    case 0x2006:
        if (internal_registers[Internal_W] == 0) // Write high byte
        {
            registers[register_index] &= 0x00FF;
            registers[register_index] |= ((int)byte << 8);
            internal_registers[Internal_T] &= 0x00FF;
            internal_registers[Internal_T] |= ((int)byte << 8); // ((byte & 0xF) << 8);
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

static int fr_counter = 0;
void tick()
{
    fr_counter++;
    if (fr_counter > 100000)
    {
        renderFrame();
        fr_counter = 0;
    }
}

void bootPPU()
{
    connect_ppu_to_bus(tick, readPPU, writePPU);
}

static int frameCount = 0;
void drawDBGScreen();
void drawTileDBG(int row, int col, unsigned char nametable_byte);
static void renderFrame()
{
    BeginDrawing();
    ClearBackground(PINK);
    const char *time_text = TextFormat("NES Emulator: %.2f FPS", roundf(1.0f / GetFrameTime()));
    SetWindowTitle(time_text);
    drawDBGScreen();
    EndDrawing();
    frameCount += 1;
}

void drawDBGScreen()
{
    for (int row = 0; row < 30; row++)
    {
        for (int col = 0; col < 32; col++)
        {
            unsigned char nametable_byte = vram[row * 32 + col];
            drawTileDBG(row, col, nametable_byte);
        }
    }
}

void drawTileDBG(int row, int col, unsigned char nametable_byte)
{
    const int pixel_size = 5;
    for (int i = 0; i < 8; i++)
    {
        int OFFSET = 0;
        unsigned char low = readBytePPU(BYTES_PER_TILE * nametable_byte + i);
        unsigned char high = readBytePPU(BYTES_PER_TILE * nametable_byte + 8 + i);

        for (int j = 0; j < 8; j++)
        {
            int mask = 1 << (7 - j);
            int val = (low & mask) | (high & mask);
            DrawRectangle(
                col * 8 * pixel_size + j * pixel_size,
                row * 8 * pixel_size + i * pixel_size,
                pixel_size,
                pixel_size,
                val == 0 ? BLACK : WHITE);
        }
    }
}
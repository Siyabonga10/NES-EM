#include "ppu.h"
#include "bus.h"
#include <assert.h>
#include <stdint.h>
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
#define W_RAM_SIZE 0x800
#define SYSTEM_PALETTE_SIZE 64
#define ATTR_BLOCK_SIZE 4
#define ATTR_SUBBLOCK_SIZE 2
#define COLORS_PER_PALETTE 4
#define OAM_SIZE 256
#define OAM_STEP 4

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
static unsigned char oam[OAM_SIZE] = {0};
static FrameData frameBuffer;
static NesColor system_palette[SYSTEM_PALETTE_SIZE] = {};

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
    return 0;
}

void handleDMA(int N)
{
    for (int i = 0; i < OAM_SIZE; i++)
    {
        oam[i] = fetchFromCPU(0x100 * N + i);
    }
}

void writePPU(int addr, unsigned char byte)
{
    int register_index = addr - 0x2000;
    assert((addr >= 0x2000 && addr < 0x4000) || addr == 0x4014);
    if (addr == 0x4014)
    {
        handleDMA(byte);
        return;
    }

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
        break;
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

void loadSystemPalette();
void bootPPU()
{
    frameBuffer.height = BASE_HEIGHT;
    frameBuffer.width = BASE_WIDTH;
    frameBuffer.is_new_frame = false;
    frameBuffer.data = malloc(sizeof(NesColor) * BASE_HEIGHT * BASE_WIDTH);

    connect_ppu_to_bus(tick, readPPU, writePPU);
    loadSystemPalette();
}

void killPPU()
{
    free(frameBuffer.data);
}

static bool was_true = false;

FrameData requestFrame()
{
    if (was_true && frameBuffer.is_new_frame)
    {
        frameBuffer.is_new_frame = false;
    }
    was_true = frameBuffer.is_new_frame;
    return frameBuffer;
}

static void renderFrame()
{
    drawDBGScreen();
    frameBuffer.is_new_frame = true;
}

void renderSprites();
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
    renderSprites();
}

/*
    1. We use the tile index to figure out which group of 4x4 tiles we are in, maybe floor div by for both the row and column?
    2. We then do modulo to figure out which sub 2x2 block we are a part of, these are numbered as follows
        0 1
        2 3
    3. Access the byte for the current 4x4 block from the attribute table
    3. Subdivide the block into 4 pieces, each 2 bytes
    4. Select the Nth 2 byte pair, where N correspondes the 2x2 block we are in
*/

static NesColor transparent_color = {.a = 0};
NesColor getPixelColorBackground(int row, int col, int pixel_value)
{
    assert(pixel_value < 4);
    int parent_row = row / ATTR_BLOCK_SIZE;
    int parent_col = col / ATTR_BLOCK_SIZE;

    int child_row = (row % ATTR_BLOCK_SIZE) / 2;
    int child_col = (col % ATTR_BLOCK_SIZE) / 2;

    // Attribute table located at the end of the tiles in the nametable
    int base_addr = 0x23C0;

    int attr_addr = base_addr + parent_row * 8 + parent_col;

    unsigned char attr_byte = vram[ppu_to_vram(attr_addr)];
    int sub_block_index = child_row * ATTR_SUBBLOCK_SIZE + child_col;
    int mask = 0b11;

    if (pixel_value == 0)
        return transparent_color;

    int palette_num = (attr_byte >> (sub_block_index * 2)) & mask;
    int color = palette_ram[palette_num * COLORS_PER_PALETTE + pixel_value];
    return system_palette[color];
}

NesColor getPixelColorSprite(unsigned char attr_byte, int pixel_value)
{
    int palette_num = attr_byte & 0x03;
    int color = palette_ram[0x10 + palette_num * COLORS_PER_PALETTE + pixel_value];
    if (pixel_value == 0)
        return transparent_color;
    return system_palette[color];
}

void drawTileDBG(int row, int col, unsigned char nametable_byte)
{
    for (int i = 0; i < TILE_SIZE; i++)
    {
        int offset = (registers[0] & 16) == 0 ? 0 : 0x1000;
        unsigned char low = readBytePPU(offset + BYTES_PER_TILE * nametable_byte + i);
        unsigned char high = readBytePPU(offset + BYTES_PER_TILE * nametable_byte + TILE_SIZE + i);

        for (int j = 0; j < TILE_SIZE; j++)
        {
            int shiftVal = (TILE_SIZE - 1 - j);
            int mask = 1 << shiftVal;
            int val = ((low & mask) >> shiftVal) | (((high & mask) >> shiftVal) << 1);
            int bufferIndex = (row * TILE_SIZE + i) * BASE_WIDTH + col * TILE_SIZE + j;
            *(frameBuffer.data + bufferIndex) = getPixelColorBackground(row, col, val);
        }
    }

    // DrawRectangleLines(col * TILE_SIZE * SCALLING_FACTOR, row * TILE_SIZE * SCALLING_FACTOR, TILE_SIZE * SCALLING_FACTOR, TILE_SIZE * SCALLING_FACTOR, LIME);
}

void renderSprites()
{
    for (int k = 0; k < OAM_SIZE / OAM_STEP; k++)
    {
        unsigned char y_coord = oam[k * OAM_STEP];
        unsigned char tile_index = oam[k * OAM_STEP + 1];
        unsigned char attributes = oam[k * OAM_STEP + 2];
        unsigned char x_coord = oam[k * OAM_STEP + 3];
        assert((registers[0] & (1 << 5)) == 0);
        for (int i = 0; i < TILE_SIZE; i++)
        {
            int offset = (registers[0] & 8) == 0 ? 0 : 0x1000;
            unsigned char low = readBytePPU(offset + BYTES_PER_TILE * tile_index + i);
            unsigned char high = readBytePPU(offset + BYTES_PER_TILE * tile_index + TILE_SIZE + i);

            for (int j = 0; j < TILE_SIZE; j++)
            {
                bool horizantal_flip_enabled = ((attributes >> 6) & 1) != 0;
                bool vertical_flip_enabled = ((attributes >> 7) & 1) != 0;
                int shiftVal = horizantal_flip_enabled ? j : (TILE_SIZE - 1 - j);
                int y_coordinate = vertical_flip_enabled ? (TILE_SIZE - 1 - i) : i;
                int mask = 1 << shiftVal;
                int val = ((low & mask) >> shiftVal) | (((high & mask) >> shiftVal) << 1);

                int bufferIndex = (y_coord + y_coordinate) * BASE_WIDTH + x_coord + j;
                *(frameBuffer.data + bufferIndex) = getPixelColorSprite(attributes, val);
            }
        }
    }
}

void loadSystemPalette()
{
    FILE *sysP = fopen("res/iNes.pal", "rb"); // TODO: Pass this in via the function call
    if (sysP == NULL)
    {
        printf("Could not load system ram, aborting program\n");
        abort();
    }

    unsigned char colorBuffer[3] = {0};
    int index = 0;
    while (fread(colorBuffer, sizeof(unsigned char), 3, sysP))
    {
        NesColor color = {.r = colorBuffer[0], .g = colorBuffer[1], .b = colorBuffer[2], .a = 0xFF};
        system_palette[index] = color;
        index += 1;
        if (index >= SYSTEM_PALETTE_SIZE)
            return;
    }
    printf("System palette loaded\n");
}

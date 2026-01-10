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
#define CYCLES_PER_FRAME 261 // Pre render counted as well
#define VISIBLE_DOTS 256
#define VISIBLE_SCAN_LINES 240
#define BYTES_PER_PIXEL 3
#define TILES_PER_ROW 32
#define TILES_PER_COLUM 30
#define TILE_SIZE 8
#define PALETTE_RAM_SIZE 32
#define BYTES_PER_TILE 16

#define W_RAM_SIZE 0x800

enum InternalReg {
    Internal_V, 
    Internal_T, 
    Internal_X,
    Internal_W
};

static int16_t registers[EXPOSED_REGISTERS_SIZE] = {0};
static unsigned char vram[W_RAM_SIZE] = {0};
static unsigned char palette_ram[] = {0};
static int16_t internal_registers[INTERNAL_REGISTER_SIZE] = {0};
static unsigned char read_buffer = {0};

int ppu_to_vram(int ppu_address)
{
    int offset = ppu_address & 0x0FFF; 
    
    if (getCatriadge()->mirroring_mode == 0)  
        return offset & 0x07FF;
    else 
    {
        int bit10 = (offset >> 10) & 1;
        int bit11 = (offset >> 11) & 1;
        return ((bit11 << 10) | (bit10 << 11) | (offset & 0x3FF)) & 0x07FF;
    }
}

int vram_to_ppu(int vram_address)
{
    vram_address &= 0x07FF;
    return 0x2000 + vram_address;
}

static void renderFrame();
// These two functions are mainly ever used by the CPU
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
        unsigned char status_reg =  (unsigned char)registers[register_index];
        registers[2] &= 0b01111111;
        return status_reg;
    case 0x2004: 
        return (unsigned char)registers[register_index];
        break;
    case 0x2007:
        unsigned char current_read_buff = read_buffer;
        read_buffer = vram[ppu_to_vram(internal_registers[Internal_V])];
        if((registers[0] & 0x4) == 0)
                internal_registers[Internal_V] ++;
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
            if(address >= 0x3F00)
                palette_ram[address % PALETTE_RAM_SIZE] = byte;
            else
                vram[ppu_to_vram(address)] = byte;
            if((registers[0] & 0x4) == 0)
                internal_registers[Internal_V] ++;
            else 
                internal_registers[Internal_V] += 32;

            break;
        case 0x2000:  case 0x2001: case 0x2003: case 0x2004: 
            registers[register_index] = byte;
            break;
        case 0x2005: 
            // TODO: Fix scrolling
            break;
        case 0x2006:
            if(internal_registers[Internal_W] == 0) // Write high byte
            {
                registers[register_index] &= 0x00FF;
                registers[register_index] |= ((int)byte << 8);
                internal_registers[Internal_T] &= 0x00FF;
                internal_registers[Internal_T] |= ((byte & 0x3F) << 8);
                internal_registers[Internal_W] = 1;
            }
            else
            {
                registers[register_index] &= 0xFF00;
                registers[register_index] |= byte;
                internal_registers[Internal_T] &= 0xFF00;
                internal_registers[Internal_T] |= byte ;
                internal_registers[Internal_V] = internal_registers[Internal_T];
                internal_registers[Internal_W] = 0;
            }
            break;
        case 0x4014:
            registers[8] = byte;
            break;

    }
}



static int scan_line = 0;
static int dot = 0;
static Image frameBuffer;

// A bunch of utility functions for the actual rendering, getting the nametable being used, fine Y, coarse X etc
int nametableIndex() { return (internal_registers[Internal_V] & 0x0C00) >> 10; }
int fineY()          { return (internal_registers[Internal_V] & 0x7000) >> 12;}
int coarseY()        { return (internal_registers[Internal_V] & 0x03E0) >> 5; }
int coarseX()        { return (internal_registers[Internal_V] & 0x001F);}


void toggle_bit(enum InternalReg reg, int index) {
    assert(index < 15);
    int mask = 1 << index;
    internal_registers[reg] ^= mask;
}
void incrementCoarseX() {
    int c_x = coarseX() + 1;
    c_x &= 0x1F;
    if(c_x == 0) toggle_bit(Internal_V, 10);
    internal_registers[Internal_V] &= ~0x001F;
    internal_registers[Internal_V] |= c_x;
}
void incrementCoarseY() {
    int c_y = coarseY() + 1;
    if(c_y == 31) {
        c_y = 0;
        toggle_bit(Internal_V, 11);
    }
    else if(c_y == 0) {
        c_y = 0;
    }
    c_y <<= 5;
    c_y &= 0x03E0;
    internal_registers[Internal_V] &= ~0x03E0;
    internal_registers[Internal_V] |= c_y;
}

void incrementFineY() {
    int fine_y = fineY() + 1;
    if(fine_y >= 8) {
        incrementCoarseY();
        fine_y = 0;
    }
    fine_y <<= 12;
    fine_y &= 0x7000;
    internal_registers[Internal_V] &= ~0x7000;
    internal_registers[Internal_V] |= fine_y;
}


unsigned char pt_low;
unsigned char pt_high;

void loadNextRow() {
    unsigned char nametable_byte = vram[ppu_to_vram(internal_registers[Internal_V])];
    pt_low = readBytePPU(BYTES_PER_TILE * nametable_byte + fineY());
    pt_high = readBytePPU(BYTES_PER_TILE * nametable_byte + 8 + fineY());
    // printf("V=%04X, vram_addr=%04X, tile=%02X\n", 
    //    internal_registers[Internal_V], 
    //    ppu_to_vram(internal_registers[Internal_V]),
    //    nametable_byte);
}

Color getPixelColor(int low, int high, int col) {
    int col_index = col % 8;
    int mask = 1 << (7 - col_index);
    int color_index = ((high & mask) ? 2 : 0) | ((low & mask) ? 1 : 0);
    return color_index == 0 ? BLACK : WHITE; // Do grayscale for now
}

static void writePixel(int row, int column, Color color) {
    assert(row < VISIBLE_SCAN_LINES && column < VISIBLE_DOTS);
    int index = VISIBLE_DOTS * row + column;
    index *= BYTES_PER_PIXEL;
    ((unsigned char*)frameBuffer.data)[index] = color.r;
    ((unsigned char*)frameBuffer.data)[index + 1] = color.g;
    ((unsigned char*)frameBuffer.data)[index + 2] = color.b;
}

void tick() {
    if (scan_line == 0 && dot == 0) {
    internal_registers[Internal_V] = 0x2000;
}
    bool can_render = scan_line< VISIBLE_SCAN_LINES && dot < VISIBLE_DOTS; 
    if(can_render)
        writePixel(scan_line, dot, getPixelColor(pt_low, pt_high, dot % TILE_SIZE));
    if (dot <= VISIBLE_DOTS && dot % 8 == 0) {
        loadNextRow();
        incrementCoarseX();
    }

    // if (dot == 257) {
    //     int mask = 0x41F;
    //     internal_registers[Internal_V] &= (~mask);
    //     internal_registers[Internal_V] |= (internal_registers[Internal_T] & mask);
    // }

    // if (scan_line == 262 && dot >= 280 && dot <= 304) {
    //     int mask = 0x7BE0;
    //     internal_registers[Internal_V] &= (~mask);
    //     internal_registers[Internal_V] |= (internal_registers[Internal_T] & mask);
    // }

    if (dot >= DOTS_PER_CYCLE) {
        dot = -1; // to be incremented at the end
        scan_line += 1;
        
        if (scan_line < 240) {
             incrementFineY();
        }

        if (scan_line >= CYCLES_PER_FRAME) {
            scan_line = 0;
            renderFrame();
        } 
    }
    dot += 1;
}

void bootPPU()
{
    connect_ppu_to_bus(tick, readPPU, writePPU);
    frameBuffer.data = malloc(3 * 256 * 240 *sizeof(unsigned char));
    frameBuffer.width = 256;
    frameBuffer.height = 240;
    frameBuffer.mipmaps = 1;
    frameBuffer.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
} 

void killPPU() {
    free(frameBuffer.data);
}

static int frameCount = 0;
static void renderFrame() {
    BeginDrawing();
    ClearBackground(PINK);
    Texture texture = LoadTextureFromImage(frameBuffer);
    Vector2 pos = {0};
    DrawTextureEx(texture, pos, 0, 3, WHITE);
    const char* time_text = TextFormat("NES Emulator: %.2f FPS", roundf(1.0f/GetFrameTime()));
    SetWindowTitle(time_text);
    EndDrawing();
    frameCount += 1;
}


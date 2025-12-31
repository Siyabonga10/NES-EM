#include "ppu.h"
#include "bus.h"
#include <assert.h>
#include <stdint.h>
#include <raylib.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#define INTERNAL_REGISTER_SIZE 4
#define EXPOSED_REGISTERS_SIZE 9
#define DOTS_PER_CYCLE 341
#define CYCLES_PER_FRAME 262
#define VISIBLE_DOTS 256
#define VISIBLE_SCAN_LINES 240
#define BYTES_PER_PIXEL 3

#define W_RAM_SIZE 0x2000

enum InternalReg {
    Internal_V, 
    Internal_T, 
    Internal_X,
    Internal_W
};

static int16_t registers[EXPOSED_REGISTERS_SIZE] = {0};
static unsigned char vram[W_RAM_SIZE] = {0};
static int16_t internal_registers[INTERNAL_REGISTER_SIZE] = {0};
static unsigned char read_buffer = {0};

static void render();
// These two functions are mainly ever used by the CPU
unsigned char readPPU(int addr)
{
    int register_index = addr - 0x2000;
    assert(register_index < EXPOSED_REGISTERS_SIZE || register_index == 0x2014);
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
        read_buffer = vram[internal_registers[Internal_V]];
        if((registers[0] & 0x2) == 0)
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
    assert(register_index < EXPOSED_REGISTERS_SIZE || register_index == 0x2014);
    switch (addr)
    {
        case 0x2007:
            registers[register_index] = byte;
            vram[internal_registers[Internal_V]] = byte;
            if((registers[0] & 0x2) == 0)
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
                registers[register_index] |= (byte << 8);
                internal_registers[Internal_T] &= 0x00FF;
                internal_registers[Internal_T] |= (byte << 8);
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


static void writeTo(int addr, unsigned char value) {
    
}

static int scan_line = -1;
static int dot = 0;
static Image frameBuffer;
// The plan is to have a texture, where we write to, on a per pixel basis, then just render that on each iteration

void tick() {
    // Render the current dot
    // udpate dot counter
    // Check if are at the end of the current row
    // we do the fetch in one clean swoop, then just sit and wait for the cursor to wrap around
    // same thing at the bottom of the screen
    dot += 1;
    if(dot >= DOTS_PER_CYCLE) {
        dot = 0;
        scan_line +=1 ;
        if(scan_line >= CYCLES_PER_FRAME) {
            scan_line = 0;
            render();
        }

    }
}

static void writePixel(int row, int column, Color color) {
    int index = VISIBLE_DOTS * row + column;
    index *= BYTES_PER_PIXEL;
    ((unsigned char*)frameBuffer.data)[index] = color.r;
    ((unsigned char*)frameBuffer.data)[index + 1] = color.g;
    ((unsigned char*)frameBuffer.data)[index + 2] = color.b;
}

void bootPPU()
{
    connect_ppu_to_bus(tick);
    frameBuffer.data = malloc(3 * 256 * 240 *sizeof(unsigned char));
    frameBuffer.width = 256;
    frameBuffer.height = 240;
    frameBuffer.mipmaps = 1;
    frameBuffer.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    for(int row = 0; row < VISIBLE_SCAN_LINES; row++) {
        for(int col = 0; col < VISIBLE_DOTS; col ++) {
            writePixel(row, col, col % 8 >= 4 ? WHITE : BLACK);
        }
    }
}

void killPPU() {
    free(frameBuffer.data);
}

static void render() {
    BeginDrawing();
    ClearBackground(PINK);
    Texture texture = LoadTextureFromImage(frameBuffer);
    Vector2 pos = {0};
    DrawTextureEx(texture, pos, 0, 3, WHITE);
    const char* time_text = TextFormat("NES Emulator: %.2f FPS", roundf(1.0f/GetFrameTime()));
    SetWindowTitle(time_text);
    EndDrawing();
}


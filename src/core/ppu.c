#include "ppu.h"
#include "bus.h"
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <raylib.h>
#include "instructions.h"

#define DEBUG_PPU 0

#if DEBUG_PPU
#define PPU_DEBUG(...)                                        \
    do                                                        \
    {                                                         \
        if (debug_frame_count < 50)                           \
        {                                                     \
            printf("[PPU %d:%d] ", current_row, current_dot); \
            printf(__VA_ARGS__);                              \
        }                                                     \
    } while (0)
#else
#define PPU_DEBUG(...)
#endif
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
static int dma_cycles_remaining = 0;
static bool dma_active = false;
static int debug_frame_count = 0;

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
static uint16_t internal_registers[INTERNAL_REGISTER_SIZE] = {0};
static unsigned char read_buffer = {0};
static unsigned char oam[OAM_SIZE] = {0};
static FrameData frameBuffer;
static NesColor system_palette[SYSTEM_PALETTE_SIZE] = {};
static int scalling_fact = 4;
static unsigned char bg_pixel_opacity[256 * 240] = {0};
static bool sprite0_hit = false;
static inline int palette_mirror(int index)
{
    index &= 0x1F;
    if (index == 0x10)
        index = 0x00;
    else if (index == 0x14)
        index = 0x04;
    else if (index == 0x18)
        index = 0x08;
    else if (index == 0x1C)
        index = 0x0C;
    return index;
}

static inline int get_nametable_base()
{
    int base = (registers[0] & 0x03) << 10;
    return 0x2000 + base;
}

static inline int get_coarse_x()
{
    return internal_registers[Internal_V] & 0x1F;
}

static inline int get_coarse_y()
{
    return (internal_registers[Internal_V] >> 5) & 0x1F;
}

static inline void check_sprite0_hit()
{
    if (sprite0_hit)
        return;
    if ((registers[1] & 0x18) != 0x18)
        return;
    if (current_row < 0 || current_row >= VISIBLE_SCAN_LINES)
        return;
    if (current_dot < 1 || current_dot > VISIBLE_DOTS)
        return;

    int sprite_height = (registers[0] & 0x20) ? 16 : 8;
    int sprite0_y = oam[0];
    int sprite0_x = oam[3];

    if (sprite0_y >= 240 || sprite0_y + sprite_height <= 0)
        return;
    if (sprite0_x >= 256 || sprite0_x + 8 <= 0)
        return;

    if (current_row < sprite0_y || current_row >= sprite0_y + sprite_height)
        return;
    if (current_dot < sprite0_x || current_dot >= sprite0_x + 8)
        return;

    int bufferIndex = current_row * BASE_WIDTH + (current_dot - 1);
    if (bg_pixel_opacity[bufferIndex] == 0)
        return;

    sprite0_hit = true;
    registers[2] |= 0x40;
    PPU_DEBUG("SPRITE0 HIT! sprite0_y=%d, sprite0_x=%d, bg_opacity=1\n", sprite0_y, sprite0_x);
}

static inline int get_nametable_select()
{
    return (internal_registers[Internal_V] >> 10) & 0x03;
}

static inline int get_tile_ppu_addr(int row, int col)
{
    int v = internal_registers[Internal_V];
    int nt = (v >> 10) & 0x03;
    int coarse_y = (v >> 5) & 0x1F;
    int coarse_x = v & 0x1F;

    int tile_y = coarse_y + row;
    int tile_x = coarse_x + col;

    while (tile_y >= 30)
    {
        tile_y -= 30;
        nt ^= 0x02; // toggle vertical nametable
    }
    while (tile_x >= 32)
    {
        tile_x -= 32;
        nt ^= 0x01; // toggle horizontal nametable
    }

    return 0x2000 + (nt << 10) + tile_y * 32 + tile_x;
}

void drawDBGScreen();
bool drawTileDBG(int row, int col, unsigned char nametable_byte);
static void renderFrame();

int ppu_to_vram(int ppu_address)
{
    if (ppu_address >= 0x2000 && ppu_address <= 0x3EFF)
    {
        ppu_address -= 0x2000;
        if (ppu_address >= 0x3000)
            ppu_address -= 0x1000; // mirror down

        int nametable_index = ppu_address / 0x400; // 0-3
        int offset_in_nametable = ppu_address % 0x400;

        Cartriadge *cart = getCatriadge();
        int mirroring = cart ? cart->mirroring_mode : 0; // default horizontal

        int vram_index;
        if (mirroring == 0)
        {                                                        // horizontal
            vram_index = (nametable_index >= 2) ? 0x400 : 0x000; // 0,1 -> A; 2,3 -> B
        }
        else
        {                                                          // vertical
            vram_index = (nametable_index & 0x01) ? 0x400 : 0x000; // 0,2 -> A; 1,3 -> B
        }

        return vram_index + offset_in_nametable;
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
        if (sprite0_hit)
        {
            status_reg |= 0x40;
            PPU_DEBUG("STATUS READ: sprite0_hit=1, returning 0x%02X\n", status_reg);
        }
        else
        {
            PPU_DEBUG("STATUS READ: sprite0_hit=0, returning 0x%02X\n", status_reg);
        }
        registers[2] &= 0b00111111;
        sprite0_hit = false;
        return status_reg;
    case 0x2004:
    {
        int addr = registers[3] & 0xFF;
        unsigned char val = oam[addr];
        registers[register_index] = val;
        PPU_DEBUG("Read OAM $2004[0x%02X] = 0x%02X\n", addr, val);
        registers[3] = (addr + 1) & 0xFF;
        return val;
    }
    case 0x2007:
        unsigned char current_read_buff = read_buffer;
        int address = internal_registers[Internal_V];
        unsigned char data;
        if (address >= 0x3F00)
        {
            int palette_index = palette_mirror(address);
            data = palette_ram[palette_index];
            PPU_DEBUG("Read PPUDATA $2007 from palette[0x%04X->0x%02X] = 0x%02X\n", address, palette_index, data);
        }
        else
        {
            data = vram[ppu_to_vram(address)];
            PPU_DEBUG("Read PPUDATA $2007 from VRAM[0x%04X] = 0x%02X (buf=0x%02X)\n", address, data, current_read_buff);
        }
        read_buffer = data;
        if ((registers[0] & 0x4) == 0)
            internal_registers[Internal_V]++;
        else
            internal_registers[Internal_V] += 32;
        return (address >= 0x3F00) ? data : current_read_buff;
    }
    return 0;
}

void handleDMA(int N)
{
    PPU_DEBUG("DMA START: page=0x%02X, cycles=513\n", N);
    dma_active = true;
    dma_cycles_remaining = 513; // 1 dummy cycle + 256 read-write cycles

    for (int i = 0; i < OAM_SIZE; i++)
    {
        oam[i] = fetchFromCPU(0x100 * N + i);
    }
}

bool is_dma_active() { return dma_active; }

void update_dma_cycles()
{
    if (dma_active)
    {
        dma_cycles_remaining--;
        if (dma_cycles_remaining <= 0)
        {
            dma_active = false;
            PPU_DEBUG("DMA END\n");
        }
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
        {
            int palette_index = palette_mirror(address);
            palette_ram[palette_index] = byte;
            PPU_DEBUG("Write PPUDATA $2007=0x%02X to palette[0x%04X->0x%02X]\n", byte, address, palette_index);
        }
        else if (address < 0x2000)
        {
            Cartriadge *cart = getCatriadge();
            if (cart)
            {
                if (cart->chr_ram != NULL)
                {
                    cart->chr_ram[address & 0x1FFF] = byte;
                    PPU_DEBUG("Write PPUDATA $2007=0x%02X to CHR-RAM[0x%04X]\n", byte, address);
                }
                else if (cart->mem)
                {
                    int offset = cart->ppuMapper(cart, address);
                    if (offset >= 0 && offset < cart->size)
                    {
                        cart->mem[offset] = byte;
                        PPU_DEBUG("Write PPUDATA $2007=0x%02X to CHR-ROM[0x%04X->0x%04X]\n", byte, address, offset);
                    }
                }
            }
        }
        else
        {
            vram[ppu_to_vram(address)] = byte;
            PPU_DEBUG("Write PPUDATA $2007=0x%02X to VRAM[0x%04X]\n", byte, address);
        }

        if ((registers[0] & 0x4) == 0)
            internal_registers[Internal_V]++;
        else
            internal_registers[Internal_V] += 32;
        break;
    case 0x2000:
        PPU_DEBUG("Write PPUCTRL $2000=0x%02X (NMI=%d, SpriteSize=%d, BGPattern=%d, SpritePattern=%d, Incr=%d, Nametable=%d)\n",
                  byte,
                  (byte >> 7) & 1,
                  (byte >> 5) & 1,
                  (byte >> 4) & 1,
                  (byte >> 3) & 1,
                  (byte >> 2) & 1,
                  byte & 3);
        bool old_nmi_output = (registers[0] & 0x80) != 0;
        registers[0] = byte;
        bool new_nmi_output = (registers[0] & 0x80) != 0;

        if (!old_nmi_output && new_nmi_output && (registers[2] & 0x80) != 0)
        {
            PPU_DEBUG("Delayed NMI triggered (writing $2000=0x%02X, vblank flag set)\n", byte);
            triggerDelayedNMI();
        }
        break;
    case 0x2001:
        PPU_DEBUG("Write PPUMASK $2001=0x%02X (BGRender=%d, SpriteRender=%d, BGLeft=%d, SpriteLeft=%d, Greyscale=%d)\n",
                  byte,
                  (byte >> 3) & 1,
                  (byte >> 4) & 1,
                  (byte >> 1) & 1,
                  (byte >> 2) & 1,
                  byte & 1);
        registers[register_index] = byte;
        break;
    case 0x2003:
        PPU_DEBUG("Write OAMADDR $2003 = 0x%02X\n", byte);
        registers[register_index] = byte;
        break;
    case 0x2004:
    {
        registers[register_index] = byte;
        int addr = registers[3];
        oam[addr] = byte;
        PPU_DEBUG("Write OAM $2004[0x%02X] = 0x%02X\n", addr, byte);
        registers[3] = (addr + 1) & 0xFF;
        break;
    }
    case 0x2005:
        PPU_DEBUG("Write PPUSCROLL $2005=0x%02X (W=%d)\n", byte, internal_registers[Internal_W]);
        if (internal_registers[Internal_W] == 0)
        {
            internal_registers[Internal_T] &= ~0x1F;
            internal_registers[Internal_T] |= (byte >> 3) & 0x1F;
            internal_registers[Internal_X] = byte & 0x07;
        }
        else
        {
            internal_registers[Internal_T] &= ~(0x1F << 5);
            internal_registers[Internal_T] |= ((byte >> 3) & 0x1F) << 5;
            internal_registers[Internal_T] &= ~(0x07 << 12);
            internal_registers[Internal_T] |= (byte & 0x07) << 12;
        }
        internal_registers[Internal_W] ^= 1;
        break;
    case 0x2006:
        PPU_DEBUG("Write PPUADDR $2006=0x%02X (W=%d)\n", byte, internal_registers[Internal_W]);
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
            PPU_DEBUG("PPUADDR complete: V=0x%04X\n", internal_registers[Internal_V]);
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
    check_sprite0_hit();
    if (current_dot == 260 && current_row < VISIBLE_SCAN_LINES)
    {
        Cartriadge *cart = getCatriadge();
        if (cart && cart->scanlineTick)
            cart->scanlineTick(cart);
    }
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
        PPU_DEBUG("VBLANK flag set (start of vblank)\n");
        registers[2] |= 0x80;
        if ((registers[0] & 0x80) != 0)
        {
            PPU_DEBUG("VBLANK NMI triggered (reg $2000=0x%02X)\n", registers[0]);
            triggerNMI();
        }
    }



    if (current_row == 261 && current_dot == 1)
    {
        if (sprite0_hit)
        {
            PPU_DEBUG("Clearing sprite0_hit at pre-render scanline\n");
        }
        PPU_DEBUG("VBLANK flag cleared (end of vblank)\n");
        registers[2] &= 0b01111111;
        sprite0_hit = false;
    }
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

FrameData *requestFrame()
{
    if (was_true && frameBuffer.is_new_frame)
    {
        frameBuffer.is_new_frame = false;
    }
    was_true = frameBuffer.is_new_frame;
    return &frameBuffer;
}

static void renderFrame()
{
    drawDBGScreen();
    frameBuffer.is_new_frame = true;
    debug_frame_count++;
}

static bool sprite_rendering_enabled = true;
void renderSprites();
void drawDBGScreen()
{
    memset(bg_pixel_opacity, 0, sizeof(bg_pixel_opacity));

    int nametable_base = get_nametable_base();
    for (int row = 0; row < TILES_PER_COLUM; row++)
    {
        for (int col = 0; col < TILES_PER_ROW; col++)
        {
            int ppu_addr = nametable_base + row * TILES_PER_ROW + col;
            unsigned char nametable_byte = vram[ppu_to_vram(ppu_addr)];
            drawTileDBG(row, col, nametable_byte);
        }
    }
    if (sprite_rendering_enabled)
        renderSprites();
    if (IsKeyPressed(KEY_R))
        sprite_rendering_enabled = !sprite_rendering_enabled;
    //  assert(false);
}

void draw_tile_indices_dbg()
{
    int nametable_base = get_nametable_base();
    for (int row = 0; row < TILES_PER_COLUM; row++)
    {
        for (int col = 0; col < TILES_PER_ROW; col++)
        {
            int ppu_addr = nametable_base + row * TILES_PER_ROW + col;
            unsigned char nametable_byte = vram[ppu_to_vram(ppu_addr)];
            bool hasSomething = drawTileDBG(row, col, 0x44);
            if (hasSomething)
            {
                const char *text = TextFormat("%x", 0x44);
                DrawText(text, (col * TILE_SIZE) * scalling_fact, (row * TILE_SIZE) * scalling_fact, 15, GREEN);
            }
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

static NesColor transparent_color = {};
NesColor getPixelColorBackground(int row, int col, int pixel_value)
{
    assert(pixel_value < 4);
    int parent_row = row / ATTR_BLOCK_SIZE;
    int parent_col = col / ATTR_BLOCK_SIZE;

    int child_row = (row % ATTR_BLOCK_SIZE) / 2;
    int child_col = (col % ATTR_BLOCK_SIZE) / 2;

    int nametable_base = get_nametable_base();
    int base_addr = nametable_base + 0x3C0;

    int attr_addr = base_addr + parent_row * 8 + parent_col;

    unsigned char attr_byte = vram[ppu_to_vram(attr_addr)];
    int sub_block_index = child_row * ATTR_SUBBLOCK_SIZE + child_col;
    int mask = 0b11;

    if (pixel_value == 0)
        return system_palette[palette_ram[palette_mirror(0)]];

    int palette_num = (attr_byte >> (sub_block_index * 2)) & mask;
    int color = palette_ram[palette_mirror(palette_num * COLORS_PER_PALETTE + pixel_value)];
    return system_palette[color];
}

NesColor getPixelColorSprite(unsigned char attr_byte, int pixel_value)
{
    int palette_num = attr_byte & 0x03;
    int color = palette_ram[palette_mirror(0x10 + palette_num * COLORS_PER_PALETTE + pixel_value)];
    if (pixel_value == 0)
        return transparent_color;
    return system_palette[color];
}

void draw_nametable_dbg()
{

    int offset = (registers[0] & 16) == 0 ? 0 : 0x1000;
    int max_tiles_per_column = ((BASE_WIDTH * scalling_fact) / (16 * scalling_fact)) - 2;
    for (int tile_index = 0; tile_index < 960; tile_index++)
    {
        for (int tile_row = 0; tile_row < 8; tile_row++)
        {
            unsigned char low = readBytePPU(offset + BYTES_PER_TILE * tile_index + tile_row);
            unsigned char high = readBytePPU(offset + BYTES_PER_TILE * tile_index + TILE_SIZE + tile_row);
            int col = tile_index % max_tiles_per_column;
            int row = tile_index / max_tiles_per_column;
            bool hasSomething = false;
            for (int j = 0; j < TILE_SIZE; j++)
            {
                int shiftVal = (TILE_SIZE - 1 - j);
                int mask = 1 << shiftVal;
                int val = ((low & mask) >> shiftVal) | (((high & mask) >> shiftVal) << 1);
                hasSomething |= val != 0;
                int bufferIndex = (row * TILE_SIZE + tile_row) * BASE_WIDTH + col * TILE_SIZE + j;
                NesColor c = getPixelColorBackground(row, col, val);
                DrawRectangle(BASE_WIDTH * scalling_fact + (2 * col * TILE_SIZE + j) * scalling_fact, (2 * row * TILE_SIZE + tile_row) * scalling_fact, scalling_fact, scalling_fact, *(Color *)(void *)&c);
            }
            if (hasSomething)
            {
                const char *text = TextFormat("%x", tile_index);
                DrawText(text, BASE_WIDTH * scalling_fact + (2 * col * TILE_SIZE) * scalling_fact, ((2 * row - 1) * TILE_SIZE) * scalling_fact, 15, GREEN);
            }
        }
    }
}
bool drawTileDBG(int row, int col, unsigned char nametable_byte)
{
    bool hasSomething = false;
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
            if ((registers[1] & 0x08) != 0)
            {
                bg_pixel_opacity[bufferIndex] = (val != 0) ? 1 : 0;
            }
        }
    }
    return hasSomething;
}

void renderSprites()
{
    for (int k = 0; k < OAM_SIZE / OAM_STEP; k++)
    {
        unsigned char y_coord = oam[k * OAM_STEP];
        unsigned char tile_index = oam[k * OAM_STEP + 1];
        unsigned char attributes = oam[k * OAM_STEP + 2];
        unsigned char x_coord = oam[k * OAM_STEP + 3];
        // assert((registers[0] & (1 << 5)) == 0);
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

                int y_pos = y_coord + y_coordinate;
                int x_pos = x_coord + j;
                int bufferIndex = y_pos * BASE_WIDTH + x_pos;

                if (k == 0 && !sprite0_hit && (registers[1] & 0x18) == 0x18)
                {
                    if (y_pos < 240 && x_pos < 256 && val != 0)
                    {
                        if (bg_pixel_opacity[bufferIndex] != 0)
                        {
                            sprite0_hit = true;
                            registers[2] |= 0x40;
                        }
                    }
                }

                NesColor color = getPixelColorSprite(attributes, val);
                if (color.a != 0)
                    *(frameBuffer.data + bufferIndex) = color;
            }
        }
    }
}
static const unsigned char system_palette_data[] = {
    // 64 colors, 3 bytes each (RGB)
    0x62,
    0x62,
    0x62,
    0x00,
    0x1F,
    0xB2,
    0x24,
    0x04,
    0xC8,
    0x52,
    0x00,
    0xB2,
    0x73,
    0x00,
    0x7C,
    0x80,
    0x00,
    0x24,
    0x73,
    0x0B,
    0x00,
    0x52,
    0x28,
    0x00,
    0x24,
    0x44,
    0x00,
    0x00,
    0x57,
    0x00,
    0x00,
    0x5C,
    0x00,
    0x00,
    0x53,
    0x24,
    0x00,
    0x3C,
    0x7C,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xAB,
    0xAB,
    0xAB,
    0x0D,
    0x57,
    0xFF,
    0x4B,
    0x30,
    0xFF,
    0x8A,
    0x13,
    0xFF,
    0xBC,
    0x08,
    0xD6,
    0xD2,
    0x12,
    0x69,
    0xC7,
    0x2E,
    0x00,
    0x9D,
    0x54,
    0x00,
    0x60,
    0x7B,
    0x00,
    0x20,
    0x98,
    0x00,
    0x00,
    0xA3,
    0x00,
    0x00,
    0x99,
    0x42,
    0x00,
    0x7D,
    0xB4,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xFF,
    0xFF,
    0xFF,
    0x53,
    0xAE,
    0xFF,
    0x90,
    0x85,
    0xFF,
    0xD3,
    0x65,
    0xFF,
    0xFF,
    0x57,
    0xFF,
    0xFF,
    0x5D,
    0xCF,
    0xFF,
    0x77,
    0x57,
    0xFA,
    0x9E,
    0x00,
    0xBD,
    0xC7,
    0x00,
    0x7A,
    0xE7,
    0x00,
    0x43,
    0xF6,
    0x11,
    0x26,
    0xEF,
    0x7E,
    0x2C,
    0xD5,
    0xF6,
    0x4E,
    0x4E,
    0x4E,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xFF,
    0xFF,
    0xFF,
    0xB6,
    0xE1,
    0xFF,
    0xCE,
    0xD1,
    0xFF,
    0xE9,
    0xC3,
    0xFF,
    0xFF,
    0xBC,
    0xFF,
    0xFF,
    0xBD,
    0xF4,
    0xFF,
    0xC6,
    0xC3,
    0xFF,
    0xD5,
    0x9A,
    0xE9,
    0xE6,
    0x81,
    0xCE,
    0xF4,
    0x81,
    0xB6,
    0xFB,
    0x9A,
    0xA9,
    0xFA,
    0xC3,
    0xA9,
    0xF0,
    0xF4,
    0xB8,
    0xB8,
    0xB8,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

void loadSystemPalette()
{
    for (int i = 0; i < SYSTEM_PALETTE_SIZE; i++)
    {
        NesColor color = {
            .r = system_palette_data[i * 3],
            .g = system_palette_data[i * 3 + 1],
            .b = system_palette_data[i * 3 + 2],
            .a = 0xFF};
        system_palette[i] = color;
    }
}
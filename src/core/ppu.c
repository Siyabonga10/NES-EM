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
#define PPU_DEBUG(...)                                    \
    do                                                    \
    {                                                     \
        printf("[PPU %d:%d] ", current_row, current_dot); \
        printf(__VA_ARGS__);                              \
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
static FrameData frame_buffer;
static NesColor system_palette[SYSTEM_PALETTE_SIZE] = {};
static int scaling_factor = 4;
static unsigned char bg_pixel_opacity[256 * 240] = {0};
static bool sprite0_hit = false;
static struct
{
    unsigned char pixel_value;
    unsigned char attributes;
} sprite_scanline[256];
static bool sprite_rendering_enabled = true;
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

    int screen_x = current_dot - 1; // 0-based pixel X
    int sprite_height = (registers[0] & 0x20) ? 16 : 8;
    int sprite0_y = oam[0] + 1; // OAM Y is scanline before sprite appears
    int sprite0_x = oam[3];
    unsigned char sprite0_tile = oam[1];
    unsigned char sprite0_attr = oam[2];

    if (sprite0_y >= 240 || sprite0_x >= 255)
        return;
    if (current_row < sprite0_y || current_row >= sprite0_y + sprite_height)
        return;
    if (screen_x < sprite0_x || screen_x >= sprite0_x + 8)
        return;

    if (screen_x < 8 && ((registers[1] & 0x02) == 0 || (registers[1] & 0x04) == 0))
        return;

    // Check BG pixel is opaque
    int bufferIndex = current_row * BASE_WIDTH + screen_x;
    if (bg_pixel_opacity[bufferIndex] == 0)
        return;

    // Check sprite pixel is opaque - fetch the actual tile data
    int row_in_sprite = current_row - sprite0_y;
    bool vert_flip = (sprite0_attr >> 7) & 1;
    bool horiz_flip = (sprite0_attr >> 6) & 1;
    int actual_row = vert_flip ? (sprite_height - 1 - row_in_sprite) : row_in_sprite;

    int pattern_table_offset;
    int tile_num;
    if (sprite_height == 8)
    {
        pattern_table_offset = (registers[0] & 8) ? 0x1000 : 0;
        tile_num = sprite0_tile;
    }
    else
    {
        pattern_table_offset = (sprite0_tile & 1) ? 0x1000 : 0;
        int top = sprite0_tile & 0xFE;
        tile_num = (actual_row < 8) ? top : (top | 1);
    }

    int tile_row = actual_row & 7;
    unsigned char low = read_byte_ppu(pattern_table_offset + tile_num * BYTES_PER_TILE + tile_row);
    unsigned char high = read_byte_ppu(pattern_table_offset + tile_num * BYTES_PER_TILE + TILE_SIZE + tile_row);

    int col_in_sprite = screen_x - sprite0_x;
    int shift = horiz_flip ? col_in_sprite : (7 - col_in_sprite);
    int sprite_val = ((low >> shift) & 1) | (((high >> shift) & 1) << 1);

    if (sprite_val == 0)
        return;

    sprite0_hit = true;
    registers[2] |= 0x40;
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

void draw_dbg_screen();
bool draw_tile_dbg(int row, int col, unsigned char nametable_byte);
static void renderFrame();
static void increment_v_vertical();
static void copy_horizontal_t_to_v();
static void copy_vertical_t_to_v();
static void render_bg_pixel(int scanline, int screen_x);
NesColor get_pixel_color_sprite(unsigned char attr_byte, int pixel_value);

int ppu_to_vram(int ppu_address)
{
    if (ppu_address >= 0x2000 && ppu_address <= 0x3EFF)
    {
        ppu_address -= 0x2000;
        if (ppu_address >= 0x1000)
            ppu_address -= 0x1000; // mirror $3000-$3EFF down to $2000-$2EFF

        int nametable_index = ppu_address / 0x400; // 0-3
        int offset_in_nametable = ppu_address % 0x400;

        Cartriadge *cart = get_cartridge();
        int mirroring = cart ? cart->mirroring_mode : 0; // default horizontal

        int vram_index;
        switch (mirroring)
        {
        case 0:                                                  // horizontal
            vram_index = (nametable_index >= 2) ? 0x400 : 0x000; // 0,1 -> A; 2,3 -> B
            break;
        case 1:                                                    // vertical
            vram_index = (nametable_index & 0x01) ? 0x400 : 0x000; // 0,2 -> A; 1,3 -> B
            break;
        case 2:                 // one-screen, lower bank
            vram_index = 0x000; // all nametables use first 1KB
            break;
        case 3:                 // one-screen, upper bank
            vram_index = 0x400; // all nametables use second 1KB
            break;
        default:
            vram_index = 0x000; // fallback
            break;
        }

        return vram_index + offset_in_nametable;
    }
    return 0;
}

unsigned char read_ppu(int addr)
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
        }

        // Only clear vblank flag (bit 7) on read. Sprite-0 hit (bit 6) stays
        // set until pre-render scanline clears it.
        registers[2] &= ~0x80;
        return status_reg;
    case 0x2004:
    {
        int addr = registers[3] & 0xFF;
        unsigned char val = oam[addr];
        registers[register_index] = val;
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
        }
        else if (address < 0x2000)
        {
            data = read_byte_ppu(address);
        }
        else
        {
            data = vram[ppu_to_vram(address)];
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

void handle_dma(int N)
{
    dma_active = true;
    dma_cycles_remaining = 513; // 1 dummy cycle + 256 read-write cycles

    for (int i = 0; i < OAM_SIZE; i++)
    {
        oam[i] = fetch_from_cpu(0x100 * N + i);
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
        }
    }
}

void write_ppu(int addr, unsigned char byte)
{
    static unsigned int temp_var_1;
    int register_index = addr - 0x2000;
    assert((addr >= 0x2000 && addr < 0x4000) || addr == 0x4014);
    if (addr == 0x4014)
    {
        handle_dma(byte);
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
            palette_ram[palette_index] = byte & 0x3F;
        }
        else if (address < 0x2000)
        {
            Cartriadge *cart = get_cartridge();
            cart->ppu_write(cart, address, byte);
        }
        else
        {
            vram[ppu_to_vram(address)] = byte;
        }

        if ((registers[0] & 0x4) == 0)
            internal_registers[Internal_V]++;
        else
            internal_registers[Internal_V] += 32;
        break;
    case 0x2000:
        bool old_nmi_output = (registers[0] & 0x80) != 0;
        registers[0] = byte;
        // Update nametable select bits in T register
        internal_registers[Internal_T] = (internal_registers[Internal_T] & ~0x0C00) | ((byte & 0x03) << 10);
        bool new_nmi_output = (registers[0] & 0x80) != 0;

        if (!old_nmi_output && new_nmi_output && (registers[2] & 0x80) != 0)
        {
            PPU_DEBUG("Delayed NMI triggered (writing $2000=0x%02X, vblank flag set), cpu clock cycles since last dmni: %ld\n", byte, get_elapsed_clock_cycles() - temp_var_1);
            temp_var_1 = get_elapsed_clock_cycles();
            trigger_delayed_nmi();
        }
        break;
    case 0x2001:
        registers[register_index] = byte;
        break;
    case 0x2003:
        registers[register_index] = byte;
        break;
    case 0x2004:
    {
        registers[register_index] = byte;
        int addr = registers[3];
        oam[addr] = byte;
        registers[3] = (addr + 1) & 0xFF;
        break;
    }
    case 0x2005:
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
        if (internal_registers[Internal_W] == 0)
        {
            registers[register_index] &= 0x00FF;
            registers[register_index] |= ((int)(byte & 0x3F) << 8);
            internal_registers[Internal_T] &= 0x00FF;
            internal_registers[Internal_T] |= ((int)(byte & 0x3F) << 8);
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

static void evaluate_sprites_for_scanline(int scanline)
{
    memset(sprite_scanline, 0, sizeof(sprite_scanline));

    int sprite_height = (registers[0] & 0x20) ? 16 : 8;

    for (int k = 0; k < 64; k++)
    {
        unsigned char y_coord = oam[k * 4];
        unsigned char tile_index = oam[k * 4 + 1];
        unsigned char attributes = oam[k * 4 + 2];
        unsigned char x_coord = oam[k * 4 + 3];

        int sprite_top = y_coord + 1;
        if (scanline < sprite_top || scanline >= sprite_top + sprite_height)
            continue;

        int row_in_sprite = scanline - sprite_top;
        bool vert_flip = (attributes >> 7) & 1;
        bool horiz_flip = (attributes >> 6) & 1;
        int actual_row = vert_flip ? (sprite_height - 1 - row_in_sprite) : row_in_sprite;

        int pattern_table_offset;
        int tile_num;
        if (sprite_height == 8)
        {
            pattern_table_offset = (registers[0] & 8) ? 0x1000 : 0;
            tile_num = tile_index;
        }
        else
        {
            pattern_table_offset = (tile_index & 1) ? 0x1000 : 0;
            int top = tile_index & 0xFE;
            tile_num = (actual_row < 8) ? top : (top | 1);
        }

        int tile_row = actual_row & 7;
        unsigned char low = read_byte_ppu(pattern_table_offset + tile_num * 16 + tile_row);
        unsigned char high = read_byte_ppu(pattern_table_offset + tile_num * 16 + 8 + tile_row);

        for (int col = 0; col < 8; col++)
        {
            int shift = horiz_flip ? col : (7 - col);
            int pixel_val = ((low >> shift) & 1) | (((high >> shift) & 1) << 1);
            int x_pos = (x_coord + col) & 0xFF;

            if (pixel_val == 0)
                continue;
            if (x_pos < 8 && (registers[1] & 0x04) == 0)
                continue;
            if (sprite_scanline[x_pos].pixel_value != 0)
                continue;

            sprite_scanline[x_pos].pixel_value = pixel_val;
            sprite_scanline[x_pos].attributes = attributes;
        }
    }
}

static void composite_sprite_pixel(int scanline, int screen_x)
{
    if (!sprite_rendering_enabled)
        return;

    unsigned char pixel = sprite_scanline[screen_x].pixel_value;
    if (pixel == 0)
        return;

    unsigned char attr = sprite_scanline[screen_x].attributes;
    bool behind_bg = (attr >> 5) & 1;
    int bufferIndex = scanline * BASE_WIDTH + screen_x;

    if (behind_bg && bg_pixel_opacity[bufferIndex] != 0)
        return;

    NesColor color = get_pixel_color_sprite(attr, pixel);
    if (color.a != 0)
        frame_buffer.data[bufferIndex] = color;
}

void tick()
{
    cycle_count++;
    current_dot++;
    static unsigned int temp_var_2;

    bool rendering_enabled = (registers[1] & 0x18) != 0;

    // Evaluate sprites at scanline start (dot 1)
    if (current_row < VISIBLE_SCAN_LINES && current_dot == 1 && sprite_rendering_enabled)
    {
        evaluate_sprites_for_scanline(current_row);
    }

    // Visible scanlines: render BG pixel, composite sprite, then check sprite0
    if (current_row < VISIBLE_SCAN_LINES && current_dot >= 1 && current_dot <= VISIBLE_DOTS)
    {
        render_bg_pixel(current_row, current_dot - 1);
        composite_sprite_pixel(current_row, current_dot - 1);
        check_sprite0_hit();
    }

    // Sprite overflow evaluation for next scanline (dot 64)
    if (current_dot == 64 && current_row < VISIBLE_SCAN_LINES && rendering_enabled)
    {
        int next_line = current_row + 1;
        int sprite_height = (registers[0] & 0x20) ? 16 : 8;
        int count = 0;
        bool overflow = false;
        for (int i = 0; i < 64; i++)
        {
            int y = oam[i * 4];
            if (y >= 0xEF)
                break;
            if (next_line >= y + 1 && next_line < y + 1 + sprite_height)
            {
                count++;
                if (count > 8)
                    overflow = true;
            }
        }
        if (overflow)
            registers[2] |= 0x20;
    }

    // V register increments during visible scanlines (and pre-render)
    if (rendering_enabled && (current_row < VISIBLE_SCAN_LINES || current_row == 261))
    {
        // Dot 256: increment vertical position in V
        if (current_dot == 256)
        {
            increment_v_vertical();
        }
        // Dot 257: copy horizontal bits from T to V
        if (current_dot == 257)
        {
            copy_horizontal_t_to_v();
        }
    }

    // Pre-render scanline: copy vertical bits from T to V (dots 280-304)
    if (current_row == 261 && rendering_enabled && current_dot >= 280 && current_dot <= 304)
    {
        copy_vertical_t_to_v();
    }

    // Mapper scanline tick (MMC3 IRQ counter) — fires at scanline start
    if (current_dot == 1 && rendering_enabled && (current_row < VISIBLE_SCAN_LINES || current_row == 261))
    {
        Cartriadge *cart = get_cartridge();
        if (cart && cart->scanline_tick)
            cart->scanline_tick(cart);
    }

    // End of scanline
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

    // VBlank start
    if (current_row == 241 && current_dot == 1)
    {
        registers[2] |= 0x80;
        if ((registers[0] & 0x80) != 0)
        {
            PPU_DEBUG("VBLANK NMI triggered (reg $2000=0x%02X), number of cpu clock cycles since last trigger: %ld\n", registers[0], get_elapsed_clock_cycles() - temp_var_2);
            temp_var_2 = get_elapsed_clock_cycles();
            trigger_nmi();
        }
    }

    // Pre-render scanline: clear flags
    if (current_row == 261 && current_dot == 1)
    {
        if (sprite0_hit)
        {
        }
        registers[2] &= 0b00011111;
        sprite0_hit = false;
        memset(bg_pixel_opacity, 0, sizeof(bg_pixel_opacity));
    }
}

void load_system_palette();
void boot_ppu()
{
    frame_buffer.height = BASE_HEIGHT;
    frame_buffer.width = BASE_WIDTH;
    frame_buffer.is_new_frame = false;
    frame_buffer.data = malloc(sizeof(NesColor) * BASE_HEIGHT * BASE_WIDTH);

    connect_ppu_to_bus(tick, read_ppu, write_ppu);
    load_system_palette();
}

void kill_ppu()
{
    free(frame_buffer.data);
}

static bool was_true = false;

FrameData *request_frame()
{
    if (was_true && frame_buffer.is_new_frame)
    {
        frame_buffer.is_new_frame = false;
    }
    was_true = frame_buffer.is_new_frame;
    return &frame_buffer;
}

static void renderFrame()
{
    draw_dbg_screen();

    unsigned char emphasis = registers[1] & 0xE0;
    if (emphasis)
    {
        for (int i = 0; i < BASE_HEIGHT * BASE_WIDTH; i++)
        {
            NesColor *c = &frame_buffer.data[i];
            if (!(emphasis & 0x20))
                c->r = c->r - (c->r >> 2);
            if (!(emphasis & 0x40))
                c->g = c->g - (c->g >> 2);
            if (!(emphasis & 0x80))
                c->b = c->b - (c->b >> 2);
        }
    }

    frame_buffer.is_new_frame = true;
    debug_frame_count++;
}

void render_sprites();

// Increment the fine Y / coarse Y / nametable vertical bit in V (dot 256)
static void increment_v_vertical()
{
    uint16_t v = internal_registers[Internal_V];
    if ((v & 0x7000) != 0x7000)
    {
        v += 0x1000; // increment fine Y
    }
    else
    {
        v &= ~0x7000;              // fine Y = 0
        int y = (v & 0x03E0) >> 5; // coarse Y
        if (y == 29)
        {
            y = 0;
            v ^= 0x0800; // toggle vertical nametable
        }
        else if (y == 31)
        {
            y = 0; // wrap without nametable toggle
        }
        else
        {
            y++;
        }
        v = (v & ~0x03E0) | (y << 5);
    }
    internal_registers[Internal_V] = v;
}

// Copy horizontal position bits from T to V (dot 257)
static void copy_horizontal_t_to_v()
{
    // Copy coarse X and nametable horizontal bit
    internal_registers[Internal_V] = (internal_registers[Internal_V] & ~0x041F) |
                                     (internal_registers[Internal_T] & 0x041F);
}

// Copy vertical position bits from T to V (pre-render scanline, dots 280-304)
static void copy_vertical_t_to_v()
{
    // Copy fine Y, coarse Y, and nametable vertical bit
    internal_registers[Internal_V] = (internal_registers[Internal_V] & 0x041F) |
                                     (internal_registers[Internal_T] & ~0x041F);
}

// Render a single background pixel at the current scanline/dot using V register
static void render_bg_pixel(int scanline, int screen_x)
{
    int bufferIndex = scanline * BASE_WIDTH + screen_x;

    if ((registers[1] & 0x08) == 0)
    {
        frame_buffer.data[bufferIndex] = system_palette[palette_ram[palette_mirror(0)]];
        bg_pixel_opacity[bufferIndex] = 0;
        return;
    }

    if (screen_x < 8 && (registers[1] & 0x02) == 0)
    {
        frame_buffer.data[bufferIndex] = system_palette[palette_ram[palette_mirror(0)]];
        bg_pixel_opacity[bufferIndex] = 0;
        return;
    }

    uint16_t v = internal_registers[Internal_V];
    int fine_x = internal_registers[Internal_X];
    int fine_y = (v >> 12) & 0x07;
    int coarse_y = (v >> 5) & 0x1F;
    int coarse_x = v & 0x1F;
    int nt = (v >> 10) & 0x03;

    // Compute which tile column, accounting for fine_x scroll
    int effective_x = screen_x + fine_x;
    int tile_offset = effective_x / 8;
    int pixel_fine_x = effective_x % 8;

    int tile_x = coarse_x + tile_offset;
    int pixel_nt = nt;
    while (tile_x >= 32)
    {
        tile_x -= 32;
        pixel_nt ^= 0x01; // toggle horizontal nametable
    }

    int pattern_base = (registers[0] & 0x10) ? 0x1000 : 0;

    // Fetch nametable byte
    int nt_addr = 0x2000 + (pixel_nt << 10) + coarse_y * 32 + tile_x;
    unsigned char tile_index = vram[ppu_to_vram(nt_addr)];

    // Fetch pattern table data
    unsigned char low = read_byte_ppu(pattern_base + tile_index * BYTES_PER_TILE + fine_y);
    unsigned char high = read_byte_ppu(pattern_base + tile_index * BYTES_PER_TILE + TILE_SIZE + fine_y);

    int shift = 7 - pixel_fine_x;
    int val = ((low >> shift) & 1) | (((high >> shift) & 1) << 1);

    if (val == 0)
    {
        frame_buffer.data[bufferIndex] = system_palette[palette_ram[palette_mirror(0)]];
        bg_pixel_opacity[bufferIndex] = 0;
    }
    else
    {
        // Attribute table lookup
        int attr_base = 0x2000 + (pixel_nt << 10) + 0x3C0;
        int attr_row = coarse_y / 4;
        int attr_col = tile_x / 4;
        unsigned char attr_byte = vram[ppu_to_vram(attr_base + attr_row * 8 + attr_col)];
        int sub_row = (coarse_y % 4) / 2;
        int sub_col = (tile_x % 4) / 2;
        int palette_num = (attr_byte >> ((sub_row * 2 + sub_col) * 2)) & 0x03;
        int color = palette_ram[palette_mirror(palette_num * COLORS_PER_PALETTE + val)];
        frame_buffer.data[bufferIndex] = system_palette[color];
        bg_pixel_opacity[bufferIndex] = 1;
    }
}

void draw_dbg_screen()
{
    if (IsKeyPressed(KEY_R))
        sprite_rendering_enabled = !sprite_rendering_enabled;
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
            bool has_something = draw_tile_dbg(row, col, 0x44);
            if (has_something)
            {
                const char *text = TextFormat("%x", 0x44);
                DrawText(text, (col * TILE_SIZE) * scaling_factor, (row * TILE_SIZE) * scaling_factor, 15, GREEN);
            }
        }
    }
    render_sprites();
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
NesColor get_pixel_color_background(int row, int col, int pixel_value)
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

NesColor get_pixel_color_sprite(unsigned char attr_byte, int pixel_value)
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
    int max_tiles_per_column = ((BASE_WIDTH * scaling_factor) / (16 * scaling_factor)) - 2;
    for (int tile_index = 0; tile_index < 960; tile_index++)
    {
        for (int tile_row = 0; tile_row < 8; tile_row++)
        {
            unsigned char low = read_byte_ppu(offset + BYTES_PER_TILE * tile_index + tile_row);
            unsigned char high = read_byte_ppu(offset + BYTES_PER_TILE * tile_index + TILE_SIZE + tile_row);
            int col = tile_index % max_tiles_per_column;
            int row = tile_index / max_tiles_per_column;
            bool has_something = false;
            for (int j = 0; j < TILE_SIZE; j++)
            {
                int shiftVal = (TILE_SIZE - 1 - j);
                int mask = 1 << shiftVal;
                int val = ((low & mask) >> shiftVal) | (((high & mask) >> shiftVal) << 1);
                has_something |= val != 0;
                int bufferIndex = (row * TILE_SIZE + tile_row) * BASE_WIDTH + col * TILE_SIZE + j;
                NesColor c = get_pixel_color_background(row, col, val);
                DrawRectangle(BASE_WIDTH * scaling_factor + (2 * col * TILE_SIZE + j) * scaling_factor, (2 * row * TILE_SIZE + tile_row) * scaling_factor, scaling_factor, scaling_factor, *(Color *)(void *)&c);
            }
            if (has_something)
            {
                const char *text = TextFormat("%x", tile_index);
                DrawText(text, BASE_WIDTH * scaling_factor + (2 * col * TILE_SIZE) * scaling_factor, ((2 * row - 1) * TILE_SIZE) * scaling_factor, 15, GREEN);
            }
        }
    }
}
bool draw_tile_dbg(int row, int col, unsigned char nametable_byte)
{
    bool has_something = false;
    for (int i = 0; i < TILE_SIZE; i++)
    {
        int offset = (registers[0] & 16) == 0 ? 0 : 0x1000;
        unsigned char low = read_byte_ppu(offset + BYTES_PER_TILE * nametable_byte + i);
        unsigned char high = read_byte_ppu(offset + BYTES_PER_TILE * nametable_byte + TILE_SIZE + i);

        for (int j = 0; j < TILE_SIZE; j++)
        {
            int shiftVal = (TILE_SIZE - 1 - j);
            int mask = 1 << shiftVal;
            int val = ((low & mask) >> shiftVal) | (((high & mask) >> shiftVal) << 1);
            int bufferIndex = (row * TILE_SIZE + i) * BASE_WIDTH + col * TILE_SIZE + j;
            *(frame_buffer.data + bufferIndex) = get_pixel_color_background(row, col, val);
            if ((registers[1] & 0x08) != 0)
            {
                bg_pixel_opacity[bufferIndex] = (val != 0) ? 1 : 0;
            }
        }
    }
    return has_something;
}

void render_sprites()
{
    int sprite_height = (registers[0] & 0x20) ? 16 : 8;

    for (int k = 0; k < OAM_SIZE / OAM_STEP; k++)
    {
        unsigned char y_coord = oam[k * OAM_STEP];
        unsigned char tile_index = oam[k * OAM_STEP + 1];
        unsigned char attributes = oam[k * OAM_STEP + 2];
        unsigned char x_coord = oam[k * OAM_STEP + 3];

        int pattern_table_offset;
        int tile_number_top, tile_number_bottom;

        if (sprite_height == 8)
        {
            // 8x8 sprite: pattern table from PPUCTRL bit 3
            pattern_table_offset = (registers[0] & 8) ? 0x1000 : 0;
            tile_number_top = tile_index;
            tile_number_bottom = tile_index; // not used
        }
        else
        {
            // 8x16 sprite: pattern table from tile index bit 0
            pattern_table_offset = (tile_index & 1) ? 0x1000 : 0;
            tile_number_top = tile_index & 0xFE;
            tile_number_bottom = tile_number_top | 1;
        }

        for (int row = 0; row < sprite_height; row++)
        {
            int tile_row = row & 7; // row within tile (0-7)
            int tile_select = (row < 8) ? tile_number_top : tile_number_bottom;

            // For vertical flip, invert row within the 16-pixel sprite
            bool vertical_flip_enabled = (attributes >> 7) & 1;
            int actual_row = vertical_flip_enabled ? (sprite_height - 1 - row) : row;
            int actual_tile_row = actual_row & 7;
            int actual_tile_select = (actual_row < 8) ? tile_number_top : tile_number_bottom;

            unsigned char low = read_byte_ppu(pattern_table_offset + BYTES_PER_TILE * actual_tile_select + actual_tile_row);
            unsigned char high = read_byte_ppu(pattern_table_offset + BYTES_PER_TILE * actual_tile_select + TILE_SIZE + actual_tile_row);

            for (int col = 0; col < 8; col++)
            {
                bool horizontal_flip_enabled = (attributes >> 6) & 1;
                int shift = horizontal_flip_enabled ? col : (7 - col);
                int mask = 1 << shift;
                int val = ((low & mask) >> shift) | (((high & mask) >> shift) << 1);

                int y_pos = y_coord + 1 + row;
                int x_pos = (x_coord + col) & 0xFF;

                if (y_pos >= 240 || val == 0)
                    continue;
                if (x_pos < 8 && (registers[1] & 0x04) == 0)
                    continue;

                int bufferIndex = y_pos * BASE_WIDTH + x_pos;

                // Check sprite priority (bit 5: 0=in front, 1=behind background)
                bool behind_background = (attributes >> 5) & 1;
                if (behind_background && bg_pixel_opacity[bufferIndex] != 0)
                    continue;

                NesColor color = get_pixel_color_sprite(attributes, val);
                if (color.a != 0)
                    *(frame_buffer.data + bufferIndex) = color;
            }
        }
    }
}

#include "sys_palette.h"

void load_system_palette()
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
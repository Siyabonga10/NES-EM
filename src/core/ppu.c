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
static FrameData frame_buffer;
static NesColor system_palette[SYSTEM_PALETTE_SIZE] = {};
static int scaling_factor = 4;
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
    PPU_DEBUG("SPRITE0 HIT! y=%d x=%d row=%d\n", sprite0_y, sprite0_x, current_row);
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

int ppu_to_vram(int ppu_address)
{
    if (ppu_address >= 0x2000 && ppu_address <= 0x3EFF)
    {
        ppu_address -= 0x2000;
        if (ppu_address >= 0x3000)
            ppu_address -= 0x1000; // mirror down

        int nametable_index = ppu_address / 0x400; // 0-3
        int offset_in_nametable = ppu_address % 0x400;

        Cartriadge *cart = get_cartridge();
        int mirroring = cart ? cart->mirroring_mode : 0; // default horizontal

        int vram_index;
        switch (mirroring)
        {
        case 0: // horizontal
            vram_index = (nametable_index >= 2) ? 0x400 : 0x000; // 0,1 -> A; 2,3 -> B
            break;
        case 1: // vertical
            vram_index = (nametable_index & 0x01) ? 0x400 : 0x000; // 0,2 -> A; 1,3 -> B
            break;
        case 2: // one-screen, lower bank
            vram_index = 0x000; // all nametables use first 1KB
            break;
        case 3: // one-screen, upper bank
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
            PPU_DEBUG("STATUS READ: sprite0_hit=1, returning 0x%02X\n", status_reg);
        }
        else
        {
            PPU_DEBUG("STATUS READ: sprite0_hit=0, returning 0x%02X\n", status_reg);
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
        PPU_DEBUG("Read OAM $2004[0x%02X] = 0x%02X\n", addr, val);
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
        else if (address < 0x2000)
        {
            data = read_byte_ppu(address);
            PPU_DEBUG("Read PPUDATA $2007 from CHR[0x%04X] = 0x%02X (buf=0x%02X)\n", address, data, current_read_buff);
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

void handle_dma(int N)
{
    PPU_DEBUG("DMA START: page=0x%02X, cycles=513\n", N);
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
            PPU_DEBUG("DMA END\n");
        }
    }
}

void write_ppu(int addr, unsigned char byte)
{
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
            palette_ram[palette_index] = byte;
            PPU_DEBUG("Write PPUDATA $2007=0x%02X to palette[0x%04X->0x%02X]\n", byte, address, palette_index);
        }
        else if (address < 0x2000)
        {
            Cartriadge *cart = get_cartridge();
            if (cart)
            {
                if (cart->chr_ram != NULL)
                {
                    int offset = cart->ppu_mapper(cart, address);
                    // For CHR-RAM, the offset from ppu_mapper may include the ROM base;
                    // we need to map it back to CHR-RAM space
                    if (cart->ch_ram_size > 0)
                        offset %= cart->ch_ram_size;
                    cart->chr_ram[offset] = byte;
                    PPU_DEBUG("Write PPUDATA $2007=0x%02X to CHR-RAM[0x%04X]\n", byte, address);
                }
                else if (cart->mem)
                {
                    int offset = cart->ppu_mapper(cart, address);
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
        // Update nametable select bits in T register
        internal_registers[Internal_T] = (internal_registers[Internal_T] & ~0x0C00) | ((byte & 0x03) << 10);
        bool new_nmi_output = (registers[0] & 0x80) != 0;

        if (!old_nmi_output && new_nmi_output && (registers[2] & 0x80) != 0)
        {
            PPU_DEBUG("Delayed NMI triggered (writing $2000=0x%02X, vblank flag set)\n", byte);
            trigger_delayed_nmi();
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

    bool rendering_enabled = (registers[1] & 0x18) != 0;

    // Visible scanlines: render BG pixel, then check sprite0
    if (current_row < VISIBLE_SCAN_LINES && current_dot >= 1 && current_dot <= VISIBLE_DOTS)
    {
        render_bg_pixel(current_row, current_dot - 1);
        check_sprite0_hit();
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

    // Mapper scanline tick (MMC3 IRQ counter)
    if (current_dot == 260 && current_row < VISIBLE_SCAN_LINES)
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
        PPU_DEBUG("VBLANK flag set (start of vblank)\n");
        registers[2] |= 0x80;
        if ((registers[0] & 0x80) != 0)
        {
            PPU_DEBUG("VBLANK NMI triggered (reg $2000=0x%02X)\n", registers[0]);
            trigger_nmi();
        }
    }

    // Pre-render scanline: clear flags
    if (current_row == 261 && current_dot == 1)
    {
        if (sprite0_hit)
        {
            PPU_DEBUG("Clearing sprite0_hit at pre-render scanline\n");
        }
        PPU_DEBUG("VBLANK flag cleared (end of vblank)\n");
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
    frame_buffer.is_new_frame = true;
    debug_frame_count++;
}

static bool sprite_rendering_enabled = true;
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
        v &= ~0x7000; // fine Y = 0
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
        // BG rendering disabled - backdrop color
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

    // Handle coarse_y >= 30 (attribute table region, shouldn't happen normally)
    int tile_y = coarse_y;
    if (tile_y >= 30)
    {
        tile_y -= 30;
        pixel_nt ^= 0x02;
    }

    int pattern_base = (registers[0] & 0x10) ? 0x1000 : 0;

    // Fetch nametable byte
    int nt_addr = 0x2000 + (pixel_nt << 10) + tile_y * 32 + tile_x;
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
        int attr_row = tile_y / 4;
        int attr_col = tile_x / 4;
        unsigned char attr_byte = vram[ppu_to_vram(attr_base + attr_row * 8 + attr_col)];
        int sub_row = (tile_y % 4) / 2;
        int sub_col = (tile_x % 4) / 2;
        int palette_num = (attr_byte >> ((sub_row * 2 + sub_col) * 2)) & 0x03;
        int color = palette_ram[palette_mirror(palette_num * COLORS_PER_PALETTE + val)];
        frame_buffer.data[bufferIndex] = system_palette[color];
        bg_pixel_opacity[bufferIndex] = 1;
    }
}

void draw_dbg_screen()
{
    // BG pixels are already rendered per-pixel during tick().
    // Here we only render sprites on top.
    if (sprite_rendering_enabled)
        render_sprites();
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
                
                int y_pos = y_coord + 1 + row; // OAM Y is scanline before sprite appears
                int x_pos = x_coord + col;
                
                if (y_pos >= 240 || x_pos >= 256 || val == 0)
                    continue;
                
                int bufferIndex = y_pos * BASE_WIDTH + x_pos;
                
                // Check sprite priority (bit 5: 0=in front, 1=behind background)
                bool behind_background = (attributes >> 5) & 1;
                if (behind_background && bg_pixel_opacity[bufferIndex] != 0)
                    continue;
                
                // Sprite-0 hit is now detected per-pixel during tick() via
                // check_sprite0_hit(). Do NOT set it here at end-of-frame,
                // as pre-render already cleared the flag.

                NesColor color = get_pixel_color_sprite(attributes, val);
                if (color.a != 0)
                    *(frame_buffer.data + bufferIndex) = color;
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
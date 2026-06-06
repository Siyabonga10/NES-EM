#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../instructions.h"
#include "mapper_register.h"
#include "../save_state/register_save_state.h"
#include "../save_state/mmc3_save_state.h"
#include <assert.h>

static unsigned char bank_select = 0;
static unsigned char banks[8]    = {0};
static bool          chr_mode    = false;
static bool          prg_mode    = false;

static unsigned char irq_latch   = 0;
static unsigned char irq_counter = 0;
static bool          irq_enabled = false;
static bool          irq_reload  = false;
static int           saved_mirroring = 0;

static void M004_CPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (addr < 0x8000) {
        if (cart->prg_ram) cart->prg_ram[addr - 0x6000] = value;
        return;
    }
    if (addr < 0xA000) {
        if (addr & 1) {
            unsigned char v = value;
            if (bank_select >= 6)
                v &= 0x3F;
            banks[bank_select] = v;
        } else {
            bank_select = value & 7;
            prg_mode    = value & 0x40;
            chr_mode    = value & 0x80;
        }
    } else if (addr < 0xC000) {
        if (!(addr & 1)) {
            cart->mirroring_mode = (value & 1) ? 0 : 1;
            saved_mirroring      = cart->mirroring_mode;
        }
    } else if (addr < 0xE000) {
        if (addr & 1)
            irq_reload = true;
        else
            irq_latch = value;
    } else {
        if (addr & 1)
            irq_enabled = true;
        else {
            irq_enabled = false;
            clear_pending_irq();
        }
    }
}

static unsigned char M004_CPU_READ(Cartriadge *cart, int addr) {
    if (addr < 0x8000) {
        if (cart->prg_ram) return cart->prg_ram[addr - 0x6000];
        return 0;
    }

    int total = cart->pg_rom_size / 0x2000;
    int bank;
    if (addr < 0xA000)
        bank = prg_mode ? (total - 2) : banks[6];
    else if (addr < 0xC000)
        bank = banks[7];
    else if (addr < 0xE000)
        bank = prg_mode ? banks[6] : (total - 2);
    else
        bank = total - 1;

    return cart->pg_rom[((bank * 0x2000) + (addr & 0x1FFF)) % cart->pg_rom_size];
}

static unsigned char M004_PPU(Cartriadge *cart, int addr) {
    unsigned char *chr    = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
    int            max_1k = cart->chr_ram ? (cart->ch_ram_size / 0x400)
                                         : (cart->ch_rom_bank_count * 8);

    int bank, offset;
    if (chr_mode == 0) {
        if (addr < 0x0800)      { bank = banks[0] & 0xFE; offset = addr; }
        else if (addr < 0x1000) { bank = banks[1] & 0xFE; offset = addr - 0x0800; }
        else if (addr < 0x1400) { bank = banks[2]; offset = addr - 0x1000; }
        else if (addr < 0x1800) { bank = banks[3]; offset = addr - 0x1400; }
        else if (addr < 0x1C00) { bank = banks[4]; offset = addr - 0x1800; }
        else                    { bank = banks[5]; offset = addr - 0x1C00; }
    } else {
        if (addr < 0x0400)      { bank = banks[2]; offset = addr; }
        else if (addr < 0x0800) { bank = banks[3]; offset = addr - 0x0400; }
        else if (addr < 0x0C00) { bank = banks[4]; offset = addr - 0x0800; }
        else if (addr < 0x1000) { bank = banks[5]; offset = addr - 0x0C00; }
        else if (addr < 0x1800) { bank = banks[0] & 0xFE; offset = addr - 0x1000; }
        else                    { bank = banks[1] & 0xFE; offset = addr - 0x1800; }
    }

    return chr[((bank % max_1k) * 0x400 + offset)];
}

static void M004_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (!cart->chr_ram) return;
    int max_1k = cart->ch_ram_size / 0x400;

    int bank, offset;
    if (chr_mode == 0) {
        if (addr < 0x0800)      { bank = banks[0] & 0xFE; offset = addr; }
        else if (addr < 0x1000) { bank = banks[1] & 0xFE; offset = addr - 0x0800; }
        else if (addr < 0x1400) { bank = banks[2]; offset = addr - 0x1000; }
        else if (addr < 0x1800) { bank = banks[3]; offset = addr - 0x1400; }
        else if (addr < 0x1C00) { bank = banks[4]; offset = addr - 0x1800; }
        else                    { bank = banks[5]; offset = addr - 0x1C00; }
    } else {
        if (addr < 0x0400)      { bank = banks[2]; offset = addr; }
        else if (addr < 0x0800) { bank = banks[3]; offset = addr - 0x0400; }
        else if (addr < 0x0C00) { bank = banks[4]; offset = addr - 0x0800; }
        else if (addr < 0x1000) { bank = banks[5]; offset = addr - 0x0C00; }
        else if (addr < 0x1800) { bank = banks[0] & 0xFE; offset = addr - 0x1000; }
        else                    { bank = banks[1] & 0xFE; offset = addr - 0x1800; }
    }

    cart->chr_ram[(bank % max_1k) * 0x400 + offset] = value;
}

static void M004_ScanlineTick(Cartriadge *cart) {
    (void)cart;
    if (irq_counter == 0 || irq_reload) {
        irq_counter = irq_latch;
        irq_reload  = false;
    } else {
        irq_counter--;
    }

    if (irq_counter == 0 && irq_enabled)
        trigger_irq();
}

static void mount_mapper_004_to_cartridge(Cartriadge *cart, iNesOneRomInfo cart_info) {
    cart->cpu_read           = M004_CPU_READ;
    cart->ppu_read           = M004_PPU;
    cart->cart_writer        = M004_CPU_WRITE;
    cart->ppu_write          = M004_PPU_WRITE;
    cart->scanline_tick      = M004_ScanlineTick;
    cart->pg_rom_bank_count  = cart_info.no_of_pg_rom_banks * 2;
    cart->pg_rom_bank_size   = 0x2000;
    cart->ch_rom_bank_count  = cart_info.no_of_ch_rom_banks;
    cart->ch_rom_bank_size   = 0x2000;
    saved_mirroring          = cart->mirroring_mode;
    cart->prg_ram            = malloc(0x2000);
    memset(cart->prg_ram, 0, 0x2000);
    cart->prg_ram_size       = 0x2000;
}

static void mmc3_save_state(Save_State_Info *save_buffer, uint32_t allowable_content_length) {
  Cartriadge *cart = get_cartridge();
  Mmc3SaveState state;
  state.bank_select = bank_select;
  memcpy(state.banks, banks, sizeof(banks));
  state.chr_mode    = chr_mode;
  state.prg_mode    = prg_mode;
  state.irq_latch   = irq_latch;
  state.irq_counter = irq_counter;
  state.irq_enabled = irq_enabled;
  state.irq_reload  = irq_reload;
  state.mirroring_mode = saved_mirroring;

  if (cart->prg_ram)
    memcpy(state.prg_ram, cart->prg_ram, 0x2000);
  if (cart->chr_ram)
    memcpy(state.chr_ram, cart->chr_ram, 0x2000);

  memset(save_buffer->section_label, 0, SECTION_LABEL_SIZE);
  strncpy(save_buffer->section_label, "MMC3", SECTION_LABEL_SIZE - 1);
  save_buffer->content_length = sizeof(Mmc3SaveState);
  assert(sizeof(Mmc3SaveState) <= allowable_content_length);
  memcpy(save_buffer->content, &state, sizeof(Mmc3SaveState));
}

static void mmc3_load_state(Save_State_Info *section_data) {
  Cartriadge *cart = get_cartridge();
  Mmc3SaveState state;
  memcpy(&state, section_data->content, sizeof(Mmc3SaveState));

  bank_select = state.bank_select;
  memcpy(banks, state.banks, sizeof(banks));
  chr_mode    = state.chr_mode;
  prg_mode    = state.prg_mode;
  irq_latch   = state.irq_latch;
  irq_counter = state.irq_counter;
  irq_enabled = state.irq_enabled;
  irq_reload  = state.irq_reload;
  saved_mirroring      = state.mirroring_mode;
  cart->mirroring_mode = state.mirroring_mode;

  if (cart->prg_ram)
    memcpy(cart->prg_ram, state.prg_ram, 0x2000);
  if (cart->chr_ram)
    memcpy(cart->chr_ram, state.chr_ram, 0x2000);
}

REGISTER_MAPPER(mount_mapper_004_to_cartridge, 4);
REGISTER_SAVE_STATE("MMC3", mmc3_save_state, mmc3_load_state);

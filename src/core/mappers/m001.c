#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include "../bus.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mapper_register.h"

static unsigned char load             = 0x10;
static int           last_write_cycle = -2;

static unsigned char control    = 0x0C;
static unsigned char chr_bank_0 = 0;
static unsigned char chr_bank_1 = 0;
static unsigned char prg_bank   = 0;

static bool prg_ram_e000_ok = true;
static bool prg_ram_a000_ok = true;
static bool is_snrom        = false;
static bool is_surom        = false;
static int  outer_prg_bank  = 0;

static const unsigned char mirroring_map[] = {2, 3, 1, 0};

static void M001_CPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
  if (addr < 0x8000) {
    if (prg_ram_e000_ok && prg_ram_a000_ok && cart->prg_ram)
      cart->prg_ram[addr - 0x6000] = value;
    return;
  }

  int cycle        = get_elapsed_clock_cycles();
  int consecutive  = (cycle == last_write_cycle + 1);
  last_write_cycle = cycle;

  if (value & 0x80) {
    load = 0x10;
    control |= 0x0C;
    return;
  }

  if (consecutive)
    return;

  bool full = load & 1;
  load      = ((load >> 1) | ((value & 1) << 4));
  if (!full)
    return;

  unsigned char data = load & 0x1F;
  load               = 0x10;

  if (addr < 0xA000) {
    control              = data;
    int mmc1_mirror      = data & 3;
    cart->mirroring_mode = mirroring_map[mmc1_mirror];
  } else if (addr < 0xC000) {
    chr_bank_0 = data;
    if (is_snrom)
      prg_ram_a000_ok = !(data & 0x10);
    if (is_surom)
      outer_prg_bank = (data & 0x10) ? 1 : 0;
  } else if (addr < 0xE000) {
    chr_bank_1 = data;
    /* SUROM: outer PRG bank from chr_bank_1 only in 4KB CHR mode */
    if (is_surom && (control & 0x10))
      outer_prg_bank = (data & 0x10) ? 1 : 0;
  } else {
    prg_bank        = data & 0x0F;
    prg_ram_e000_ok = !(data & 0x10);
  }
}

static unsigned char M001_CPU_READ(Cartriadge *cart, int addr) {
  unsigned char result;
  int orig_addr = addr;

  if (addr < 0x8000) {
    if (prg_ram_e000_ok && prg_ram_a000_ok && cart->prg_ram)
      return cart->prg_ram[addr - 0x6000];
    return 0xFF;
  }
  addr                       = addr - 0x8000;
  uint32_t outer             = (uint32_t)outer_prg_bank * 0x40000;
  int      prg_rom_mode_mask = 0b01100;
  int      mode              = (control & prg_rom_mode_mask) >> 2;

  int prg_bank_no;
  switch (mode) {
  case 0x00:
  case 0x01:
    prg_bank_no = (0b1111 & prg_bank) >> 1;
    result = cart->pg_rom[(outer + (0x8000 * prg_bank_no) + addr) % cart->pg_rom_size];
    break;
  case 0x02:
    if (addr < 0x4000)
      result = cart->pg_rom[(outer + addr) % cart->pg_rom_size];
    else {
      prg_bank_no = 0b1111 & prg_bank;
      result = cart->pg_rom[(outer + (0x4000 * prg_bank_no) + (addr - 0x4000)) % cart->pg_rom_size];
    }
    break;
  case 0x03:
    if (addr >= 0x4000)
      result = cart->pg_rom[(outer + ((uint32_t)(cart->pg_rom_bank_count - 1) * 0x4000) + (addr - 0x4000)) % cart->pg_rom_size];
    else {
      prg_bank_no = 0b1111 & prg_bank;
      result = cart->pg_rom[(outer + (0x4000 * prg_bank_no) + addr) % cart->pg_rom_size];
    }
    break;
  default:
    return 0;
  }
  return result;
}

static unsigned char M001_PPU(Cartriadge *cart, int addr) {
  unsigned char *chr    = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
  int            max_4k = cart->chr_ram ? (cart->ch_ram_size / 0x1000)
                                        : (cart->ch_rom_bank_count * 2);

  int chr_mode = (control >> 4) & 1;
  if (chr_mode == 0) {
    int bank = (chr_bank_0 >> 1) % (max_4k / 2);
    return chr[bank * 0x2000 + addr];
  }
  if (addr < 0x1000) {
    int bank = chr_bank_0 % max_4k;
    return chr[bank * 0x1000 + addr];
  }
  int bank = chr_bank_1 % max_4k;
  return chr[bank * 0x1000 + (addr - 0x1000)];
}

static void M001_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
  unsigned char *chr = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
  if (!cart->chr_ram)
    return;
  int max_4k   = cart->ch_ram_size / 0x1000;
  int chr_mode = (control >> 4) & 1;
  if (chr_mode == 0) {
    int bank                  = (chr_bank_0 >> 1) % (max_4k / 2);
    chr[bank * 0x2000 + addr] = value;
  } else if (addr < 0x1000) {
    int bank                  = chr_bank_0 % max_4k;
    chr[bank * 0x1000 + addr] = value;
  } else {
    int bank                             = chr_bank_1 % max_4k;
    chr[bank * 0x1000 + (addr - 0x1000)] = value;
  }
}

static void mount_mapper_001_to_cartridge(Cartriadge *cart, iNesOneRomInfo cart_info) {
  is_snrom       = (strncmp(cart_info.board_type, "NES-SNROM", 9) == 0);
  is_surom       = (strncmp(cart_info.board_type, "NES-SUROM", 9) == 0) || (strncmp(cart_info.board_type, "NES-SXROM", 9) == 0);
  outer_prg_bank = 0;

  cart->cpu_read          = M001_CPU_READ;
  cart->ppu_read          = M001_PPU;
  cart->cart_writer       = M001_CPU_WRITE;
  cart->ppu_write         = M001_PPU_WRITE;
  cart->pg_rom_bank_count = is_surom ? 16 : cart_info.no_of_pg_rom_banks;
  cart->ch_rom_bank_count = cart_info.no_of_ch_rom_banks;
  cart->pg_rom_bank_size  = 0x4000;
  cart->ch_rom_bank_size  = cart_info.no_of_ch_rom_banks == 0 ? 0 : 0x2000;
  if (is_snrom || is_surom || cart_info.has_pg_ram) {
    cart->prg_ram      = malloc(0x2000);
    memset(cart->prg_ram, 0, 0x2000);
    cart->prg_ram_size = 0x2000;
  } else {
    cart->prg_ram      = NULL;
    cart->prg_ram_size = 0;
  }
}

REGISTER_MAPPER(mount_mapper_001_to_cartridge, 1);

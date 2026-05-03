#include "m001.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define BIT_7_MASK 0x80
#define REGISTER_SELECT_MASK 0b0110000000000000
#define LOAD_REGISTER_MASK 0b00011111
#define BASE 0x8000
#define MAX_WRITES 5

static unsigned char load = 0x10;
static int write_count = 0;

static unsigned char control = 0x0C;
static unsigned char chr_bank_0 = 0;
static unsigned char chr_bank_1 = 0;
static unsigned char prg_bank = 0;


static const unsigned char mirroring_map[] = {2, 3, 1, 0};

void M001_Write(Cartriadge *cart, int addr, unsigned char value)
{
  if (value & 0x80) // Reset
  {
    load = 0x10;
    control |= 0x0C;
    return;
  }
  bool full = load & 1;
  load = ((load >> 1) | ((value & 1) << 4));
  if (!full)
    return;

  unsigned char data = load & 0x1F;
  load = 0x10;

  if (addr < 0xA000) {
    control = data;
    int mmc1_mirror = data & 3;
    cart->mirroring_mode = mirroring_map[mmc1_mirror];
  }
  else if (addr < 0xC000)
    chr_bank_0 = data;
  else if (addr < 0xE000)
    chr_bank_1 = data;
  else
    prg_bank = data & 0x0F;
  return;
}

int M001(Cartriadge *cart, int addr)
{
  if(addr < 0x8000) {
    assert(addr >= 0x6000);
    if(addr >= 0x6000) {
      return addr - 0x6000;
    }
  }
  addr = addr - 0x8000;
  int prg_rom_mode_mask = 0b01100;
  int mode = (control & prg_rom_mode_mask) >> 2;

  int prg_bank_no;

  switch (mode)
  {
  case 0x00: // fallthrough
  case 0x01:
    prg_bank_no = (0b1111 & prg_bank) >> 1;
    return ((0x8000 * prg_bank_no) + addr) % cart->pg_rom_size;
  case 0x02:
    if(addr < 0x4000) {
      return addr % cart->pg_rom_size;
    }
    prg_bank_no = 0b1111 & prg_bank;
    return ((0x4000 * prg_bank_no) + (addr - 0x4000)) % cart->pg_rom_size;
  case 0x03:
    if(addr >= 0x4000) {
      return ((cart->pg_rom_bank_count - 1) * 0x4000 + (addr - 0x4000)) % cart->pg_rom_size;
    }
    prg_bank_no = 0b1111 & prg_bank;
    return ((0x4000 * prg_bank_no) + addr) % cart->pg_rom_size;
    
  default:
    break;
  }
  return 0;
}

unsigned char M001_PPU(Cartriadge *cart, int addr)
{
  unsigned char *chr = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
  int max_4k = cart->chr_ram ? (cart->ch_ram_size / 0x1000)
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

void M001_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
  unsigned char *chr = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
  if (!cart->chr_ram) return;
  int max_4k = cart->ch_ram_size / 0x1000;
  int chr_mode = (control >> 4) & 1;
  if (chr_mode == 0) {
    int bank = (chr_bank_0 >> 1) % (max_4k / 2);
    chr[bank * 0x2000 + addr] = value;
  } else if (addr < 0x1000) {
    int bank = chr_bank_0 % max_4k;
    chr[bank * 0x1000 + addr] = value;
  } else {
    int bank = chr_bank_1 % max_4k;
    chr[bank * 0x1000 + (addr - 0x1000)] = value;
  }
}

#include "m001.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define BIT_7_MASK 0b10000000
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


static void reset()
{
  load = 0x10;
  write_count = 0;
}

static const unsigned char mirroring_map[] = {2, 3, 1, 0};

void M001_Write(Cartriadge *cart, int addr, unsigned char value)
{
  if (value & BIT_7_MASK)
  {
    load = 0x10 | (value & 1);
    write_count = 1;
    return;
  }

  load |= ((value & 1) << write_count++);
  if (write_count == MAX_WRITES)
  {
    int selected_register = (addr & REGISTER_SELECT_MASK) >> 13;
    reset();
    switch (selected_register)
    {
      case 0x00: control    = load & LOAD_REGISTER_MASK; cart->mirroring_mode = mirroring_map[load & 3]; break;
      case 0x01: chr_bank_0 = load & LOAD_REGISTER_MASK; break;
      case 0x02: chr_bank_1 = load & LOAD_REGISTER_MASK; break;
      case 0x03: prg_bank   = load & LOAD_REGISTER_MASK; break;

      default: fprintf(stderr, "[M0001] Could not resolve selected register\n"); abort();
    }
  }
}

int M001(Cartriadge *cart, int addr)
{
  addr = addr - 0x8000;
  int prg_rom_mode_mask = 0b01100;
  int mode = (control & prg_rom_mode_mask) >> 2;

  int prg_bank_no;

  switch (mode)
  {
  case 0x00: // fallthrough
  case 0x01:
    prg_bank_no = (0b1111 & prg_bank) >> 1;
    return (0x8000 * prg_bank_no) + addr;
  case 0x02:
    if(addr < 0x4000) {
      return addr;
    }
    prg_bank_no = 0b1111 & prg_bank;
    return (0x4000 * prg_bank_no) + (addr - 0x4000);
  case 0x03:
    if(addr >= 0x4000) {
      return ((cart->pg_rom_bank_count - 1) * 0x4000) + (addr - 0x4000);
    }
    prg_bank_no = 0b1111 & prg_bank;
    return (0x4000 * prg_bank_no) + addr;
    
  default:
    break;
  }
  return 0;
}

unsigned char M001_PPU(Cartriadge *cart, int addr)
{
  if (cart->chr_ram)
    return cart->chr_ram[addr % 0x2000];

  int chr_mode_select_mask = 0b10000;
  int mode = control & chr_mode_select_mask;
  int stride;
  if(mode == 0) {
    stride = 0x2000;
    int bank_select = chr_bank_0 >> 1;
    return cart->ch_rom[stride * bank_select + addr];
  } else {
    stride = 0x1000;
    if(addr >= 0x1000)   return cart->ch_rom[stride * chr_bank_1 + addr - 0x1000];
    else                 return cart->ch_rom[stride * chr_bank_0 + addr];
  }
}

void M001_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
  if (cart->chr_ram)
    cart->chr_ram[addr % 0x2000] = value;
}

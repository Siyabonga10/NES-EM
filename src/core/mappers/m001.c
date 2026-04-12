#include "m001.h"
#include <stdbool.h>

static unsigned char shift_register = 0x10;
static unsigned char control = 0x0C; // PRG fix last bank mode by default
static unsigned char chr_bank_0 = 0;
static unsigned char chr_bank_1 = 0;
static unsigned char prg_bank = 0;

void M001_Write(Cartriadge *cart, int addr, unsigned char value)
{
  if (value & 0x80) // Reset
  {
    shift_register = 0x10;
    control |= 0x0C;
    return;
  }
  bool full = shift_register & 1;
  shift_register = ((shift_register >> 1) | ((value & 1) << 4));
  if (!full)
    return;

  unsigned char data = shift_register & 0x1F;
  shift_register = 0x10;

  if (addr < 0xA000)
    control = data;
  else if (addr < 0xC000)
    chr_bank_0 = data;
  else if (addr < 0xE000)
    chr_bank_1 = data;
  else
    prg_bank = data & 0x0F;
}

int M001(Cartriadge *cart, int addr)
{
  int prg_mode = (control >> 2) & 3;
  if (addr >= 0x8000 && addr <= 0xBFFF)
  {
    if (prg_mode == 2)
      return (addr - 0x8000) + 0x2000;
    if (prg_mode == 3)
      return (addr - 0x8000) + prg_bank * 0x4000 + 0x2000;
    return (addr - 0x8000) + (prg_bank & 0xFE) * 0x4000 + 0x2000;
  }
  if (addr >= 0xC000)
  {
    int last_bank = (cart->pg_rom_size / 0x4000) - 1;
    if (prg_mode == 2)
      return (addr - 0xC000) + prg_bank * 0x4000 + 0x2000;
    if (prg_mode == 3)
      return (addr - 0xC000) + last_bank * 0x4000 + 0x2000;
    return (addr - 0xC000) + (prg_bank | 1) * 0x4000 + 0x2000;
  }
  return addr - 0x6000; // WRAM
}

int M001_PPU(Cartriadge *cart, int addr)
{
  int chr_mode = (control >> 4) & 1;
  int base = cart->pg_rom_size + 0x2000;
  if (chr_mode == 0) // 8KB mode
    return base + (chr_bank_0 & 0xFE) * 0x1000 + addr;
  if (addr < 0x1000)
    return base + chr_bank_0 * 0x1000 + addr;
  return base + chr_bank_1 * 0x1000 + (addr - 0x1000);
}

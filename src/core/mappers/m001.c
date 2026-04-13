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

  if (addr < 0xA000) {
    control = data;
    // MMC1 mirroring: 0=one-screen lower, 1=one-screen upper, 2=vertical, 3=horizontal
    // Map to PPU mirroring: 0=horizontal, 1=vertical, 2=one-screen lower, 3=one-screen upper
    int mmc1_mirror = data & 3;
    switch (mmc1_mirror) {
      case 0: cart->mirroring_mode = 2; break; // one-screen lower
      case 1: cart->mirroring_mode = 3; break; // one-screen upper
      case 2: cart->mirroring_mode = 1; break; // vertical
      case 3: cart->mirroring_mode = 0; break; // horizontal
    }
  }
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
  int chr_rom_size = cart->size - cart->pg_rom_size - 0x2000;
  int base = chr_rom_size > 0 ? cart->pg_rom_size + 0x2000 : 0;
  
  // Determine max banks based on CHR-ROM or CHR-RAM size
  int max_4k = 0;
  if (chr_rom_size > 0) {
    max_4k = chr_rom_size / 0x1000;
  } else if (cart->chr_ram != 0) {
    max_4k = cart->ch_ram_size / 0x1000;
  }
  
  if (chr_mode == 0) { // 8KB mode
    int bank = chr_bank_0 & 0xFE;
    if (max_4k > 0) bank %= max_4k;
    return base + bank * 0x1000 + addr;
  }
  // 4KB mode
  if (addr < 0x1000) {
    int bank = chr_bank_0;
    if (max_4k > 0) bank %= max_4k;
    return base + bank * 0x1000 + addr;
  }
  int bank = chr_bank_1;
  if (max_4k > 0) bank %= max_4k;
  return base + bank * 0x1000 + (addr - 0x1000);
}

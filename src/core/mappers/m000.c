#include "m000.h"

int M000(Cartriadge *cart, int addr)
{
  if (addr < 0x8000)
    return addr - 0x6000;

  int mapped = addr - 0x8000;
  if (cart->pg_rom_size <= 0x4000)
  {
    mapped %= 0x4000;
  }

  return mapped + 0x2000;
}

int M000_PPU(Cartriadge *cart, int addr)
{
  int chr_rom_size = cart->size - cart->pg_rom_size - 0x2000;
  if (chr_rom_size > 0) {
    return cart->pg_rom_size + 0x2000 + addr;
  } else {
    return addr; // CHR-RAM at beginning of cart->mem
  }
}

void NO_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
}
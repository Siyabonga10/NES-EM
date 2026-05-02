#include "m000.h"
#include <stdio.h>

int M000(Cartriadge *cart, int addr)
{
  if (addr < 0x8000)
    return addr - 0x6000;

  int mapped = addr - 0x8000;
  if (cart->pg_rom_size <= 0x4000)
  {
    mapped %= 0x4000;
  }
  return mapped;
}

int M000_PPU(Cartriadge *cart, int addr)
{
  return addr;
}

void NO_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
}
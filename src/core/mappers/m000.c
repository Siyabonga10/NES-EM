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

unsigned char M000_PPU(Cartriadge *cart, int addr)
{
  return cart->ch_rom[addr % 0x2000];
}

void NO_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
}

void M000_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
  if (cart->chr_ram)
    cart->chr_ram[addr % 0x2000] = value;
}
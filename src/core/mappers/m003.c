#include "m003.h"

static unsigned char chr_bank = 0;

void M003_Write(Cartriadge *cart, int addr, unsigned char value)
{
    chr_bank = value & 0x03;
}

int M003(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int mapped = addr - 0x8000;
    if (cart->pg_rom_size <= 0x4000)
        mapped %= 0x4000;

    return mapped + 0x2000;
}

int M003_PPU(Cartriadge *cart, int addr)
{
    return cart->pg_rom_size + 0x2000 + (chr_bank * 0x2000) + addr;
}
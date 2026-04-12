#include "m007.h"

static unsigned char prg_bank = 0;

void M007_Write(Cartriadge *cart, int addr, unsigned char value)
{
    prg_bank = value & 0x0F;
    cart->mirroring_mode = (value >> 4) & 1;
}

int M007(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int offset = addr & 0x7FFF;
    return (prg_bank * 0x8000) + offset + 0x2000;
}

int M007_PPU(Cartriadge *cart, int addr)
{
    return cart->pg_rom_size + 0x2000 + addr;
}
#include "m005.h"

static unsigned char prg_banks[4];
static unsigned char chr_banks[8];

void M005_Write(Cartriadge *cart, int addr, unsigned char value)
{
    if (addr >= 0x5000 && addr < 0x5010)
    {
        int reg = addr & 0x0F;
        if (reg < 4)
            prg_banks[reg] = value;
        else if (reg < 12)
            chr_banks[reg - 4] = value;
    }
}

int M005(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int window = (addr >> 13) & 3;
    int bank = prg_banks[window];
    int offset = addr & 0x1FFF;
    return (bank * 0x2000) + offset + 0x2000;
}

int M005_PPU(Cartriadge *cart, int addr)
{
    int window = addr >> 10;
    if (window >= 8)
        window = 7;
    int bank = chr_banks[window];
    int offset = addr & 0x03FF;
    return cart->pg_rom_size + 0x2000 + (bank * 0x0400) + offset;
}
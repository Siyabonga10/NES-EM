#include "m006.h"

static unsigned char prg_bank = 0;
static unsigned char mirroring = 0;

void M006_Write(Cartriadge *cart, int addr, unsigned char value)
{
    prg_bank = value & 0x0F;
    mirroring = (value >> 4) & 1;
    cart->mirroring_mode = mirroring;
}

int M006(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int bank = prg_bank;
    int offset = addr & 0x7FFF;
    return (bank * 0x8000) + offset + 0x2000;
}

int M006_PPU(Cartriadge *cart, int addr)
{
    return cart->pg_rom_size + 0x2000 + addr;
}
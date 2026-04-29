#include "m005.h"

static unsigned char prg_banks[4];
static unsigned char chr_banks[8];

void M005_Write(Cartriadge *cart, int addr, unsigned char value)
{
    if (addr >= 0x5114 && addr <= 0x5117) {
        prg_banks[addr - 0x5114] = value & 0x7F;
    } else if (addr >= 0x5120 && addr <= 0x5127) {
        chr_banks[addr - 0x5120] = value;
    }
}

int M005(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int window = (addr >> 13) & 3;
    int max_8k = cart->pg_rom_size / 0x2000;
    int bank = prg_banks[window] & 0x7F;
    if (max_8k > 0) bank %= max_8k;
    int offset = addr & 0x1FFF;
    return (bank * 0x2000) + offset + 0x2000;
}

int M005_PPU(Cartriadge *cart, int addr)
{
    int chr_rom_size = cart->size - cart->pg_rom_size - 0x2000;
    int window = addr >> 10;
    if (window >= 8) window = 7;
    int bank = chr_banks[window];
    int num_1k = chr_rom_size / 0x400;
    if (num_1k > 0) bank %= num_1k;
    int offset = addr & 0x03FF;
    return cart->pg_rom_size + 0x2000 + (bank * 0x0400) + offset;
}
#include "m002.h"

static unsigned char prg_bank = 0;

void M002_Write(Cartriadge *cart, int addr, unsigned char value)
{
    prg_bank = value & 0x0F;
}

int M002(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int banks = cart->pg_rom_size / 0x4000;
    int last_bank = banks - 1;

    if (addr < 0xC000)
        return (prg_bank * 0x4000) + (addr - 0x8000) + 0x2000;
    else
        return (last_bank * 0x4000) + (addr - 0xC000) + 0x2000;
}

int M002_PPU(Cartriadge *cart, int addr)
{
    int chr_rom_size = cart->size - cart->pg_rom_size - 0x2000;
    if (chr_rom_size > 0) {
        return cart->pg_rom_size + 0x2000 + addr;
    } else {
        return addr; // CHR-RAM at beginning of cart->mem
    }
}
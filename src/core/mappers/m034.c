#include "m034.h"

static unsigned char prg_bank = 0;

void M034_Write(Cartriadge *cart, int addr, unsigned char value)
{
    prg_bank = value;
}

int M034(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int prg_bank_size = 0x8000; // 32KB
    int max_prg_banks = cart->pg_rom_size / prg_bank_size;
    if (max_prg_banks == 0) max_prg_banks = 1;
    int bank = prg_bank % max_prg_banks;
    int offset = addr & 0x7FFF;
    return bank * prg_bank_size + offset + 0x2000;
}

int M034_PPU(Cartriadge *cart, int addr)
{
    // CHR is RAM, mapped linearly after PRG-ROM
    return cart->pg_rom_size + 0x2000 + addr;
}
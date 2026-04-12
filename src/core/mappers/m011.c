#include "m011.h"

static unsigned char reg = 0;

void M011_Write(Cartriadge *cart, int addr, unsigned char value)
{
    reg = value;
}

int M011(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int prg_bank = (reg >> 4) & 0x0F; // high 4 bits
    int prg_bank_size = 0x8000; // 32KB
    int max_prg_banks = cart->pg_rom_size / prg_bank_size;
    if (max_prg_banks == 0) max_prg_banks = 1;
    prg_bank %= max_prg_banks;
    int offset = addr & 0x7FFF;
    return prg_bank * prg_bank_size + offset + 0x2000;
}

int M011_PPU(Cartriadge *cart, int addr)
{
    int chr_bank = reg & 0x0F; // low 4 bits
    int chr_bank_size = 0x2000; // 8KB
    int chr_rom_size = cart->size - cart->pg_rom_size - 0x2000; // CHR-ROM size
    int max_chr_banks = chr_rom_size / chr_bank_size;
    if (max_chr_banks == 0) max_chr_banks = 1;
    chr_bank %= max_chr_banks;
    int base = cart->pg_rom_size + 0x2000;
    return base + chr_bank * chr_bank_size + addr;
}
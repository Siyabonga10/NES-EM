#include "m066.h"

static unsigned char reg = 0;

void M066_Write(Cartriadge *cart, int addr, unsigned char value)
{
    reg = value;
}

int M066(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int prg_bank = (reg >> 4) & 0x03; // bits 5-4? assume bits 5-4 are at positions 5-4, but we have 8-bit register, typical: bits 4-5? Let's shift right 4 and mask 3.
    int prg_bank_size = 0x8000; // 32KB
    int max_prg_banks = cart->pg_rom_size / prg_bank_size;
    if (max_prg_banks == 0) max_prg_banks = 1;
    prg_bank %= max_prg_banks;
    int offset = addr & 0x7FFF;
    return prg_bank * prg_bank_size + offset + 0x2000;
}

int M066_PPU(Cartriadge *cart, int addr)
{
    int chr_bank = reg & 0x03; // bits 1-0
    int chr_bank_size = 0x2000; // 8KB
    int chr_rom_size = cart->size - cart->pg_rom_size - 0x2000; // CHR-ROM size
    int max_chr_banks = chr_rom_size / chr_bank_size;
    if (max_chr_banks == 0) max_chr_banks = 1;
    chr_bank %= max_chr_banks;
    int base = cart->pg_rom_size + 0x2000;
    return base + chr_bank * chr_bank_size + addr;
}
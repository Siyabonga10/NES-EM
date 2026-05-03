#include "m003.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

static unsigned char chr_bank = 0;

void M003_Write(Cartriadge *cart, int addr, unsigned char value)
{
    chr_bank = value & (cart->ch_rom_bank_count - 1);
}

int M003(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;
    int mapped = addr - 0x8000;
    if (cart->pg_rom_size <= 0x4000)
        mapped %= 0x4000;
    return mapped;
}

unsigned char M003_PPU(Cartriadge *cart, int addr)
{
    return cart->ch_rom[(chr_bank * 0x2000) + addr];
}

void M003_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
    if (cart->chr_ram)
        cart->chr_ram[addr % 0x2000] = value;
}
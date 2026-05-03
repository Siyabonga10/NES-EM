#include "m002.h"
#include <stdio.h>
#include <assert.h>

static unsigned char prg_bank = 0;

void M002_Write(Cartriadge *cart, int addr, unsigned char value)
{
    prg_bank = value & 0x0F;
}

int M002(Cartriadge *cart, int addr)
{
    if (addr >= 0x8000 && addr < 0x8000 + cart->pg_rom_bank_size)
        return (prg_bank * 0x4000) + (addr - 0x8000);
    else
        return ((cart->pg_rom_bank_count - 1) * 0x4000) + (addr - 0xC000);
}

unsigned char M002_PPU(Cartriadge *cart, int addr)
{
    if (cart->chr_ram)
        return cart->chr_ram[addr % 0x2000];
}

void M002_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
    if (cart->chr_ram)
        cart->chr_ram[addr % 0x2000] = value;
}
unsigned char m002_get_prg_bank(void) { return prg_bank; }
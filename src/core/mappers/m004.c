#include "m004.h"
#include <stdbool.h>

static unsigned char bank_select = 0;
static unsigned char chr_banks[8];
static unsigned char prg_banks[4];
static bool chr_mode = false;
static bool prg_mode = false;

void M004_Write(Cartriadge *cart, int addr, unsigned char value)
{
    if (addr < 0xA000)
    {
        if (addr % 2 == 0)
            bank_select = value & 0x07;
        else
        {
            if (bank_select < 2)
                prg_banks[bank_select] = value & 0x3F;
            else
                chr_banks[bank_select - 2] = value & 0xFF;
        }
    }
    else if (addr < 0xC000)
    {
        if (addr % 2 == 0)
            cart->mirroring_mode = value & 1;
    }
    else if (addr < 0xE000)
    {
        if (addr % 2 == 0)
            chr_mode = value & 0x80;
        else
            prg_mode = value & 0x40;
    }
}

int M004(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int bank;
    if (addr < 0xA000)
        bank = prg_banks[0];
    else if (addr < 0xC000)
        bank = prg_banks[1];
    else if (addr < 0xE000)
        bank = (cart->pg_rom_size / 0x2000) - 2;
    else
        bank = (cart->pg_rom_size / 0x2000) - 1;

    int offset = addr & 0x1FFF;
    return (bank * 0x2000) + offset + 0x2000;
}

int M004_PPU(Cartriadge *cart, int addr)
{
    int bank;
    if (chr_mode)
    {
        if (addr < 0x0400)
            bank = chr_banks[2];
        else if (addr < 0x0800)
            bank = chr_banks[3];
        else if (addr < 0x0C00)
            bank = chr_banks[4];
        else if (addr < 0x1000)
            bank = chr_banks[5];
        else if (addr < 0x1400)
            bank = chr_banks[0];
        else if (addr < 0x1800)
            bank = chr_banks[1];
        else if (addr < 0x1C00)
            bank = chr_banks[6];
        else
            bank = chr_banks[7];
    }
    else
    {
        if (addr < 0x0800)
            bank = chr_banks[0];
        else if (addr < 0x1000)
            bank = chr_banks[1];
        else if (addr < 0x1400)
            bank = chr_banks[2];
        else if (addr < 0x1800)
            bank = chr_banks[3];
        else if (addr < 0x1C00)
            bank = chr_banks[4];
        else
            bank = chr_banks[5];
    }
    int offset = addr & 0x03FF;
    return cart->pg_rom_size + 0x2000 + (bank * 0x0400) + offset;
}
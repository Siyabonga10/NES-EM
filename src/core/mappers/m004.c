#include "m004.h"
#include <stdbool.h>
#include <stdio.h>

#define DEBUG_MAPPER 0
#if DEBUG_MAPPER
#define MAPPER_DEBUG(...) fprintf(stderr, "[M004] " __VA_ARGS__)
#else
#define MAPPER_DEBUG(...)
#endif

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
        {
            bank_select = value & 0x07;
            chr_mode = value & 0x80;
            prg_mode = value & 0x40;
            MAPPER_DEBUG("$%04X = 0x%02X: bank_select=%d, chr_mode=%d, prg_mode=%d\n",
                         addr, value, bank_select, chr_mode ? 1 : 0, prg_mode ? 1 : 0);
        }
        else
        {
            if (bank_select >= 6)
            {
                prg_banks[bank_select - 6] = value & 0x3F;
                MAPPER_DEBUG("$%04X = 0x%02X: PRG bank R%d = 0x%02X\n",
                             addr, value, bank_select, value & 0x3F);
            }
            else
            {
                chr_banks[bank_select] = value & 0xFF;
                MAPPER_DEBUG("$%04X = 0x%02X: CHR bank R%d = 0x%02X\n",
                             addr, value, bank_select, value & 0xFF);
            }
        }
    }
    else if (addr < 0xC000)
    {
        if (addr % 2 == 0)
        {
            cart->mirroring_mode = value & 1;
            MAPPER_DEBUG("$%04X = 0x%02X: mirroring=%d\n", addr, value, value & 1);
        }
    }
    else if (addr < 0xE000)
    {
        // IRQ latch/reload registers (not implemented)
        MAPPER_DEBUG("$%04X = 0x%02X: IRQ register (ignored)\n", addr, value);
    }
}

int M004(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int prg_bank_count = cart->pg_rom_size / 0x2000;
    int bank;

    if (addr < 0xA000)
        bank = prg_mode ? (prg_bank_count - 2) : prg_banks[0];
    else if (addr < 0xC000)
        bank = prg_banks[1];
    else if (addr < 0xE000)
        bank = prg_mode ? prg_banks[0] : (prg_bank_count - 2);
    else
        bank = prg_bank_count - 1;

    int offset = addr & 0x1FFF;
    return (bank * 0x2000) + offset + 0x2000;
}

int M004_PPU(Cartriadge *cart, int addr)
{
    int chr_rom_size = cart->size - cart->pg_rom_size - 0x2000;
    int base = chr_rom_size > 0 ? cart->pg_rom_size + 0x2000 : 0;
    int bank_index;
    int offset;

    if (chr_mode == 0)
    {
        // chr_mode = 0: 2KB banks at $0000 and $0800, 1KB banks at $1000-$1FFF
        if (addr < 0x0800)
        {
            // 2KB bank using R0
            bank_index = chr_banks[0] & 0xFE;
            offset = addr & 0x07FF;
        }
        else if (addr < 0x1000)
        {
            // 2KB bank using R1
            bank_index = chr_banks[1] & 0xFE;
            offset = (addr - 0x0800) & 0x07FF;
        }
        else if (addr < 0x1400)
        {
            // 1KB bank using R2
            bank_index = chr_banks[2];
            offset = (addr - 0x1000) & 0x03FF;
        }
        else if (addr < 0x1800)
        {
            // 1KB bank using R3
            bank_index = chr_banks[3];
            offset = (addr - 0x1400) & 0x03FF;
        }
        else if (addr < 0x1C00)
        {
            // 1KB bank using R4
            bank_index = chr_banks[4];
            offset = (addr - 0x1800) & 0x03FF;
        }
        else // addr < 0x2000
        {
            // 1KB bank using R5
            bank_index = chr_banks[5];
            offset = (addr - 0x1C00) & 0x03FF;
        }
    }
    else
    {
        // chr_mode = 1: 1KB banks at $0000-$0FFF, 2KB banks at $1000 and $1800
        if (addr < 0x0400)
        {
            // 1KB bank using R2
            bank_index = chr_banks[2];
            offset = addr & 0x03FF;
        }
        else if (addr < 0x0800)
        {
            // 1KB bank using R3
            bank_index = chr_banks[3];
            offset = (addr - 0x0400) & 0x03FF;
        }
        else if (addr < 0x0C00)
        {
            // 1KB bank using R4
            bank_index = chr_banks[4];
            offset = (addr - 0x0800) & 0x03FF;
        }
        else if (addr < 0x1000)
        {
            // 1KB bank using R5
            bank_index = chr_banks[5];
            offset = (addr - 0x0C00) & 0x03FF;
        }
        else if (addr < 0x1800)
        {
            // 2KB bank using R0
            bank_index = chr_banks[0] & 0xFE;
            offset = (addr - 0x1000) & 0x07FF;
        }
        else // addr < 0x2000
        {
            // 2KB bank using R1
            bank_index = chr_banks[1] & 0xFE;
            offset = (addr - 0x1800) & 0x07FF;
        }
    }

    return base + (bank_index * 0x0400) + offset;
}
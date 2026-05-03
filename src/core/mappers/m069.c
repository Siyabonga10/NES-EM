#include "m069.h"
// NOTE: Mapper 69 (FME-7 / Sunsoft 5B) — written but not working. Untested, suspect register init or IRQ.
#include <stdbool.h>
#include "../instructions.h"

static unsigned char reg_index = 0;
static unsigned char regs[16] = {0,0,0,0,0,0,0,0, 0,0xFF,0xFF,0xFF,0xFF,0,0,0};
static unsigned char irq_counter = 0;
static bool irq_enabled = false;
static bool irq_triggered = false;

void M069_Write(Cartriadge *cart, int addr, unsigned char value)
{
    if (addr < 0xA000)
        reg_index = value & 0x0F;
    else if (addr < 0xC000)
    {
        regs[reg_index] = value;
        if (reg_index == 0x0D)
            cart->mirroring_mode = (value & 3) ^ 1;
    }
}

int M069(Cartriadge *cart, int addr)
{
    if (addr < 0x6000)
        return addr - 0x4000; // never hit, bus filters

    if (addr < 0x8000)
    {
        int bank = regs[8] & 0x3F;
        return ((bank * 0x2000) + (addr - 0x6000)) % cart->pg_rom_size;
    }

    int bank;
    if (addr < 0xA000)
        bank = regs[9];
    else if (addr < 0xC000)
        bank = regs[0xA];
    else if (addr < 0xE000)
        bank = regs[0xB];
    else
        bank = regs[0xC];

    return ((bank * 0x2000) + (addr & 0x1FFF)) % cart->pg_rom_size;
}

unsigned char M069_PPU(Cartriadge *cart, int addr)
{
    unsigned char *chr = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
    int bank = regs[addr / 0x400];
    int max_1k = cart->chr_ram ? (cart->ch_ram_size / 0x400)
                               : (cart->ch_rom_bank_count * 8);
    return chr[((bank % max_1k) * 0x400) + (addr & 0x3FF)];
}

void M069_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
    if (!cart->chr_ram) return;
    int bank = regs[addr / 0x400];
    int max_1k = cart->ch_ram_size / 0x400;
    cart->chr_ram[((bank % max_1k) * 0x400) + (addr & 0x3FF)] = value;
}

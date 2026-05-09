#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include <stdio.h>
#include <assert.h>
#include "mapper_register.h"

static unsigned char prg_bank = 0;

static void M002_Write(Cartriadge *cart, int addr, unsigned char value)
{
    prg_bank = value & 0x0F;
}

static int M002(Cartriadge *cart, int addr)
{
    if (addr >= 0x8000 && addr < 0x8000 + cart->pg_rom_bank_size)
        return (prg_bank * 0x4000) + (addr - 0x8000);
    else
        return ((cart->pg_rom_bank_count - 1) * 0x4000) + (addr - 0xC000);
}

static unsigned char M002_PPU(Cartriadge *cart, int addr)
{
    unsigned char *chr = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
    return chr[addr % 0x2000];
}

static void M002_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
    if (cart->chr_ram)
        cart->chr_ram[addr % 0x2000] = value;
}

static void mount_mapper_002_to_cartridge(Cartriadge* cart, iNesOneRomInfo cart_info) {
    cart->mapper = M002;
    cart->ppu_read = M002_PPU;
    cart->cart_writer = M002_Write;
    cart->ppu_write = M002_PPU_WRITE;
    cart->pg_rom_bank_count = cart_info.no_of_pg_rom_banks;
    cart->ch_rom_bank_count = -1;
    cart->pg_rom_bank_size = 0x4000;
    cart->ch_rom_bank_size = 0x2000;
}

REGISTER_MAPPER(mount_mapper_002_to_cartridge, 2);

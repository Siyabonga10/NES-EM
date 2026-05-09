#include "m066.h"
// NOTE: Mapper 66 (GxROM) — not tested against real games yet.

static unsigned char prg_bank = 0;
static unsigned char chr_bank = 0;

void M066_Write(Cartriadge *cart, int addr, unsigned char value)
{
    prg_bank = (value >> 4) & 3;
    chr_bank = value & 3;
}

int M066(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;
    return ((prg_bank * 0x8000) + (addr - 0x8000)) % cart->pg_rom_size;
}

unsigned char M066_PPU(Cartriadge *cart, int addr)
{
    unsigned char *chr = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
    return chr[(chr_bank * 0x2000) + addr];
}

void M066_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
    if (cart->chr_ram)
        cart->chr_ram[addr % 0x2000] = value;
}

void mount_mapper_006_to_cartridge(Cartriadge* cart, iNesOneRomInfo cart_info) {
    cart->mapper = M066;
    cart->ppu_read = M066_PPU;
    cart->cart_writer = M066_Write;
    cart->ppu_write = M066_PPU_WRITE;
    cart->pg_rom_bank_count = cart_info.no_of_pg_rom_banks;
    cart->ch_rom_bank_count = cart_info.no_of_ch_rom_banks;
    cart->pg_rom_bank_size = 0x8000;
    cart->ch_rom_bank_size = 0x2000;
}

#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include "mapper_register.h"

static unsigned char prg_bank = 0;

static void M007_CPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (addr < 0x8000) {
        if (cart->prg_ram) cart->prg_ram[addr - 0x6000] = value;
        return;
    }
    prg_bank               = value & 0x07;
    cart->mirroring_mode   = (value & 0x10) ? 1 : 0;
}

static unsigned char M007_CPU_READ(Cartriadge *cart, int addr) {
    if (addr < 0x8000) {
        if (cart->prg_ram) return cart->prg_ram[addr - 0x6000];
        return 0;
    }
    return cart->pg_rom[((prg_bank * 0x8000) + (addr - 0x8000)) % cart->pg_rom_size];
}

static unsigned char M007_PPU(Cartriadge *cart, int addr) {
    unsigned char *chr = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
    return chr[addr % 0x2000];
}

static void M007_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (cart->chr_ram)
        cart->chr_ram[addr % 0x2000] = value;
}

static void mount_mapper_007_to_cartridge(Cartriadge *cart, iNesOneRomInfo cart_info) {
    cart->cpu_read           = M007_CPU_READ;
    cart->ppu_read           = M007_PPU;
    cart->cart_writer        = M007_CPU_WRITE;
    cart->ppu_write          = M007_PPU_WRITE;
    cart->pg_rom_bank_count  = cart_info.no_of_pg_rom_banks;
    cart->ch_rom_bank_count  = -1;
    cart->pg_rom_bank_size   = 0x8000;
    cart->ch_rom_bank_size   = 0x2000;
}

REGISTER_MAPPER(mount_mapper_007_to_cartridge, 7);

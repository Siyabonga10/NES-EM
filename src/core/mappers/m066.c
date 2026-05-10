#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include "mapper_register.h"

static unsigned char prg_bank = 0;
static unsigned char chr_bank = 0;

static void M066_CPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (addr < 0x8000) {
        if (cart->prg_ram) cart->prg_ram[addr - 0x6000] = value;
        return;
    }
    prg_bank = (value >> 4) & 3;
    chr_bank = value & 3;
}

static unsigned char M066_CPU_READ(Cartriadge *cart, int addr) {
    if (addr < 0x8000) {
        if (cart->prg_ram) return cart->prg_ram[addr - 0x6000];
        return 0;
    }
    return cart->pg_rom[((prg_bank * 0x8000) + (addr - 0x8000)) % cart->pg_rom_size];
}

static unsigned char M066_PPU(Cartriadge *cart, int addr) {
    unsigned char *chr      = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
    int            chr_size = cart->chr_ram ? cart->ch_ram_size
                                           : cart->ch_rom_bank_count * 0x2000;
    return chr[((chr_bank * 0x2000) + addr) % chr_size];
}

static void M066_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (cart->chr_ram)
        cart->chr_ram[addr % 0x2000] = value;
}

static void mount_mapper_066_to_cartridge(Cartriadge *cart, iNesOneRomInfo cart_info) {
    cart->cpu_read           = M066_CPU_READ;
    cart->ppu_read           = M066_PPU;
    cart->cart_writer        = M066_CPU_WRITE;
    cart->ppu_write          = M066_PPU_WRITE;
    cart->pg_rom_bank_count  = cart_info.no_of_pg_rom_banks;
    cart->ch_rom_bank_count  = cart_info.no_of_ch_rom_banks;
    cart->pg_rom_bank_size   = 0x8000;
    cart->ch_rom_bank_size   = 0x2000;
}

REGISTER_MAPPER(mount_mapper_066_to_cartridge, 66);

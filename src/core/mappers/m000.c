#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include <stdio.h>
#include "mapper_register.h"

static int M000(Cartriadge *cart, int addr)
{
  if (addr < 0x8000)
    return addr - 0x6000;

  int mapped = addr - 0x8000;
  if (cart->pg_rom_size <= 0x4000)
  {
    mapped %= 0x4000;
  }
  return mapped;
}

static unsigned char M000_PPU(Cartriadge *cart, int addr)
{
  return cart->ch_rom[addr % 0x2000];
}

static void NO_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
}

static void M000_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
  if (cart->chr_ram)
    cart->chr_ram[addr % 0x2000] = value;
}

static void mount_mapper_000_to_cartridge(Cartriadge* cart, iNesOneRomInfo cart_info) {
  cart->mapper = M000;
  cart->ppu_read = M000_PPU;
  cart->cart_writer = NO_WRITE;
  cart->ppu_write = M000_PPU_WRITE;
  cart->pg_rom_bank_size = 0x4000 * cart_info.no_of_pg_rom_banks;
  cart->pg_rom_bank_count = -1;

  cart->ch_ram_size = 0x2000 * cart_info.no_of_ch_rom_banks;
  cart->ch_rom_bank_count = -1;
}

REGISTER_MAPPER(mount_mapper_000_to_cartridge, 0);

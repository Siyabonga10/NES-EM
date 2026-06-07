#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "../save_state/register_save_state.h"
#include "../save_state/cnrom_save_state.h"
#include "mapper_register.h"
#include <string.h>
#include <assert.h>

static unsigned char chr_bank = 0;

static void M003_CPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (addr < 0x8000) {
        if (cart->prg_ram) cart->prg_ram[addr - 0x6000] = value;
        return;
    }
    chr_bank = value & (cart->ch_rom_bank_count - 1);
}

static unsigned char M003_CPU_READ(Cartriadge *cart, int addr) {
    if (addr < 0x8000) {
        if (cart->prg_ram) return cart->prg_ram[addr - 0x6000];
        return 0;
    }
    int mapped = addr - 0x8000;
    if (cart->pg_rom_size <= 0x4000)
        mapped %= 0x4000;
    return cart->pg_rom[mapped];
}

static unsigned char M003_PPU(Cartriadge *cart, int addr) {
    return cart->ch_rom[(chr_bank * 0x2000) + addr];
}

static void M003_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (cart->chr_ram)
        cart->chr_ram[addr % 0x2000] = value;
}

static void mount_mapper_003_to_cartridge(Cartriadge *cart, iNesOneRomInfo cart_info) {
    cart->cpu_read           = M003_CPU_READ;
    cart->ppu_read           = M003_PPU;
    cart->cart_writer        = M003_CPU_WRITE;
    cart->ppu_write          = M003_PPU_WRITE;
    cart->pg_rom_bank_count  = cart_info.no_of_pg_rom_banks;
    cart->pg_rom_bank_size   = 0x4000;
    cart->ch_rom_bank_count  = cart_info.no_of_ch_rom_banks;
    cart->ch_rom_bank_size   = 0x2000;
}

static void cnrom_save_state(Save_State_Info *save_buffer, uint32_t allowable_content_length) {
  Cartriadge *cart = get_cartridge();
  CnromSaveState state;
  state.chr_bank = chr_bank;
  if (cart->prg_ram)
    memcpy(state.prg_ram, cart->prg_ram, 0x2000);

  memset(save_buffer->section_label, 0, SECTION_LABEL_SIZE);
  strncpy(save_buffer->section_label, "CNRM", SECTION_LABEL_SIZE - 1);
  save_buffer->content_length = sizeof(CnromSaveState);
  assert(sizeof(CnromSaveState) <= allowable_content_length);
  memcpy(save_buffer->content, &state, sizeof(CnromSaveState));
}

static void cnrom_load_state(Save_State_Info *section_data) {
  Cartriadge *cart = get_cartridge();
  CnromSaveState state;
  memcpy(&state, section_data->content, sizeof(CnromSaveState));
  chr_bank = state.chr_bank;
  if (cart->prg_ram)
    memcpy(cart->prg_ram, state.prg_ram, 0x2000);
}

REGISTER_MAPPER(mount_mapper_003_to_cartridge, 3);
REGISTER_SAVE_STATE("CNRM", cnrom_save_state, cnrom_load_state);

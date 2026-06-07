#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include <stdio.h>
#include <assert.h>
#include "mapper_register.h"
#include "../save_state/register_save_state.h"
#include "../save_state/uxrom_save_state.h"
#include <string.h>
#include <assert.h>

static unsigned char prg_bank = 0;

static void M002_CPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (addr < 0x8000) {
        if (cart->prg_ram) cart->prg_ram[addr - 0x6000] = value;
        return;
    }
    prg_bank = value & 0x0F;
}

static unsigned char M002_CPU_READ(Cartriadge *cart, int addr) {
    if (addr < 0x8000) {
        if (cart->prg_ram) return cart->prg_ram[addr - 0x6000];
        return 0;
    }
    int offset;
    if (addr < 0xC000)
        offset = (prg_bank * 0x4000) + (addr - 0x8000);
    else
        offset = ((cart->pg_rom_bank_count - 1) * 0x4000) + (addr - 0xC000);
    return cart->pg_rom[offset % cart->pg_rom_size];
}

static unsigned char M002_PPU(Cartriadge *cart, int addr) {
    unsigned char *chr = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
    return chr[addr % 0x2000];
}

static void M002_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (cart->chr_ram)
        cart->chr_ram[addr % 0x2000] = value;
}

static void mount_mapper_002_to_cartridge(Cartriadge *cart, iNesOneRomInfo cart_info) {
    cart->cpu_read           = M002_CPU_READ;
    cart->ppu_read           = M002_PPU;
    cart->cart_writer        = M002_CPU_WRITE;
    cart->ppu_write          = M002_PPU_WRITE;
    cart->pg_rom_bank_count  = cart_info.no_of_pg_rom_banks;
    cart->ch_rom_bank_count  = -1;
    cart->pg_rom_bank_size   = 0x4000;
    cart->ch_rom_bank_size   = 0x2000;
}


static inline bool mapper_is_active(void) {
  Cartriadge *cart = get_cartridge();
  return cart && cart->cpu_read == M002_CPU_READ;
}

static void uxrom_save_state(Save_State_Info *save_buffer, uint32_t allowable_content_length) {
  if (!mapper_is_active()) { save_buffer->content_length = 0; return; }
  Cartriadge *cart = get_cartridge();
  UxromSaveState state;
  state.prg_bank = prg_bank;
  if (cart->prg_ram)
    memcpy(state.prg_ram, cart->prg_ram, 0x2000);

  memset(save_buffer->section_label, 0, SECTION_LABEL_SIZE);
  strncpy(save_buffer->section_label, "UXRM", SECTION_LABEL_SIZE - 1);
  save_buffer->content_length = sizeof(UxromSaveState);
  assert(sizeof(UxromSaveState) <= allowable_content_length);
  memcpy(save_buffer->content, &state, sizeof(UxromSaveState));
}

static void uxrom_load_state(Save_State_Info *section_data) {
  if (!mapper_is_active()) return;
  Cartriadge *cart = get_cartridge();
  UxromSaveState state;
  memcpy(&state, section_data->content, sizeof(UxromSaveState));
  prg_bank = state.prg_bank;
  if (cart->prg_ram)
    memcpy(cart->prg_ram, state.prg_ram, 0x2000);
}

REGISTER_MAPPER(mount_mapper_002_to_cartridge, 2);
REGISTER_SAVE_STATE("UXRM", uxrom_save_state, uxrom_load_state);

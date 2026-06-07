#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include "mapper_register.h"
#include "../save_state/register_save_state.h"
#include "../save_state/axrom_save_state.h"
#include <string.h>
#include <assert.h>

static unsigned char prg_bank       = 0;
static int           ax_mirroring   = 0;

static void M007_CPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (addr < 0x8000) {
        if (cart->prg_ram) cart->prg_ram[addr - 0x6000] = value;
        return;
    }
    prg_bank               = value & 0x07;
    cart->mirroring_mode   = (value & 0x10) ? 3 : 2;
    ax_mirroring           = cart->mirroring_mode;
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
    ax_mirroring             = cart->mirroring_mode;
}

static void axrom_save_state(Save_State_Info *save_buffer, uint32_t allowable_content_length) {
  Cartriadge *cart = get_cartridge();
  AxromSaveState state;
  state.prg_bank       = prg_bank;
  state.mirroring_mode = ax_mirroring;
  if (cart->prg_ram)
    memcpy(state.prg_ram, cart->prg_ram, 0x2000);
  if (cart->chr_ram)
    memcpy(state.chr_ram, cart->chr_ram, 0x2000);

  memset(save_buffer->section_label, 0, SECTION_LABEL_SIZE);
  strncpy(save_buffer->section_label, "AXRM", SECTION_LABEL_SIZE - 1);
  save_buffer->content_length = sizeof(AxromSaveState);
  assert(sizeof(AxromSaveState) <= allowable_content_length);
  memcpy(save_buffer->content, &state, sizeof(AxromSaveState));
}

static void axrom_load_state(Save_State_Info *section_data) {
  Cartriadge *cart = get_cartridge();
  AxromSaveState state;
  memcpy(&state, section_data->content, sizeof(AxromSaveState));
  prg_bank             = state.prg_bank;
  ax_mirroring         = state.mirroring_mode;
  cart->mirroring_mode = state.mirroring_mode;
  if (cart->prg_ram)
    memcpy(cart->prg_ram, state.prg_ram, 0x2000);
  if (cart->chr_ram)
    memcpy(cart->chr_ram, state.chr_ram, 0x2000);
}

REGISTER_MAPPER(mount_mapper_007_to_cartridge, 7);
REGISTER_SAVE_STATE("AXRM", axrom_save_state, axrom_load_state);

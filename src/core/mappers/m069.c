#include "../cartriadge.h"
#include "../ines_one_rom_info.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "../instructions.h"
#include "mapper_register.h"
#include "../save_state/register_save_state.h"
#include "../save_state/fme7_save_state.h"
#include <string.h>
#include <assert.h>

static unsigned char reg_index = 0;
static unsigned char regs[16]  = {0};
static uint16_t      irq_counter = 0;

static void irq_tick(void) {
    if (!(regs[0xD] & 0x80)) return;  /* Counter Enable */
    if (!(regs[0xD] & 0x01)) return;  /* IRQ Enable */

    irq_counter--;
    if (irq_counter == 0xFFFF)
        trigger_irq();
}

static void M069_CPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    irq_tick();

    if (addr < 0x8000) {
        if ((regs[8] & 0xC0) == 0xC0 && cart->prg_ram)
            cart->prg_ram[addr - 0x6000] = value;
        return;
    }
    if (addr < 0xA000) {
        reg_index = value & 0x0F;
        return;
    }
    /* Parameter register: $A000-$BFFF */
    if (reg_index == 0x0D) {
        clear_pending_irq();     /* any write to $D acknowledges IRQ */
        regs[0xD] = value;
        if (!(value & 0x01))
            clear_pending_irq();
        return;
    }
    if (reg_index == 0x0E) {
        irq_counter = (irq_counter & 0xFF00) | value;
        regs[0xE] = value;
        return;
    }
    if (reg_index == 0x0F) {
        irq_counter = (irq_counter & 0x00FF) | ((uint16_t)value << 8);
        regs[0xF] = value;
        return;
    }
    regs[reg_index] = value;
    if (reg_index == 0x0C)
        cart->mirroring_mode = (value & 3) ^ 1;
}

static unsigned char M069_CPU_READ(Cartriadge *cart, int addr) {
    irq_tick();

    if (addr < 0x6000)
        return 0;

    if (addr < 0x8000) {
        if ((regs[8] & 0xC0) == 0xC0) {
            if (cart->prg_ram) return cart->prg_ram[addr - 0x6000];
            return 0;
        }
        int bank = regs[8] & 0x3F;
        return cart->pg_rom[((bank * 0x2000) + (addr - 0x6000)) % cart->pg_rom_size];
    }

    int bank;
    if (addr < 0xA000)
        bank = regs[9];
    else if (addr < 0xC000)
        bank = regs[0xA];
    else if (addr < 0xE000)
        bank = regs[0xB];
    else
        bank = (cart->pg_rom_size / 0x2000) - 1;

    return cart->pg_rom[((bank * 0x2000) + (addr & 0x1FFF)) % cart->pg_rom_size];
}

static unsigned char M069_PPU(Cartriadge *cart, int addr) {
    unsigned char *chr    = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
    int            bank   = regs[addr / 0x400];
    int            max_1k = cart->chr_ram ? (cart->ch_ram_size / 0x400)
                                         : (cart->ch_rom_bank_count * 8);
    return chr[((bank % max_1k) * 0x400) + (addr & 0x3FF)];
}

static void M069_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value) {
    if (!cart->chr_ram)
        return;
    int bank                                                  = regs[addr / 0x400];
    int max_1k                                                = cart->ch_ram_size / 0x400;
    cart->chr_ram[((bank % max_1k) * 0x400) + (addr & 0x3FF)] = value;
}

static void mount_mapper_069_to_cartridge(Cartriadge *cart, iNesOneRomInfo cart_info) {
    cart->cpu_read           = M069_CPU_READ;
    cart->ppu_read           = M069_PPU;
    cart->cart_writer        = M069_CPU_WRITE;
    cart->ppu_write          = M069_PPU_WRITE;
    cart->pg_rom_bank_count  = cart_info.no_of_pg_rom_banks * 2;
    cart->ch_rom_bank_count  = cart_info.no_of_ch_rom_banks * 8;
    cart->pg_rom_bank_size   = 0x2000;
    cart->ch_rom_bank_size   = 0x400;
    cart->prg_ram            = malloc(0x2000);
    cart->prg_ram_size       = 0x2000;
}

static void fme7_save_state(Save_State_Info *save_buffer, uint32_t allowable_content_length) {
  Cartriadge *cart = get_cartridge();
  Fme7SaveState state;
  state.reg_index   = reg_index;
  memcpy(state.regs, regs, sizeof(regs));
  state.irq_counter = irq_counter;

  if (cart->prg_ram)
    memcpy(state.prg_ram, cart->prg_ram, 0x2000);
  if (cart->chr_ram)
    memcpy(state.chr_ram, cart->chr_ram, 0x2000);

  memset(save_buffer->section_label, 0, SECTION_LABEL_SIZE);
  strncpy(save_buffer->section_label, "FME7", SECTION_LABEL_SIZE - 1);
  save_buffer->content_length = sizeof(Fme7SaveState);
  assert(sizeof(Fme7SaveState) <= allowable_content_length);
  memcpy(save_buffer->content, &state, sizeof(Fme7SaveState));
}

static void fme7_load_state(Save_State_Info *section_data) {
  Cartriadge *cart = get_cartridge();
  Fme7SaveState state;
  memcpy(&state, section_data->content, sizeof(Fme7SaveState));

  reg_index   = state.reg_index;
  memcpy(regs, state.regs, sizeof(regs));
  irq_counter = state.irq_counter;
  cart->mirroring_mode = (regs[0x0C] & 3) ^ 1;

  if (cart->prg_ram)
    memcpy(cart->prg_ram, state.prg_ram, 0x2000);
  if (cart->chr_ram)
    memcpy(cart->chr_ram, state.chr_ram, 0x2000);
}

REGISTER_MAPPER(mount_mapper_069_to_cartridge, 69);
REGISTER_SAVE_STATE("FME7", fme7_save_state, fme7_load_state);

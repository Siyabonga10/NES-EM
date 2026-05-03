#include "m004.h"
#include <stdbool.h>
#include "../instructions.h"

static unsigned char bank_select = 0;
static unsigned char banks[8] = {0};
static bool chr_mode = false;
static bool prg_mode = false;

static unsigned char irq_latch = 0;
static unsigned char irq_counter = 0;
static bool irq_enabled = false;
static bool irq_reload = false;

void M004_Write(Cartriadge *cart, int addr, unsigned char value)
{
  if (addr < 0xA000)
  {
    if (addr & 1)
    {
      unsigned char v = value;
      if (bank_select >= 6)
        v &= 0x3F;
      banks[bank_select] = v;
    }
    else
    {
      bank_select = value & 7;
      prg_mode = value & 0x40;
      chr_mode = value & 0x80;
    }
  }
  else if (addr < 0xC000)
  {
    if (!(addr & 1))
      cart->mirroring_mode = value & 1;
  }
  else if (addr < 0xE000)
  {
    if (addr & 1)
      irq_reload = true;
    else
      irq_latch = value;
  }
  else
  {
    if (addr & 1)
      irq_enabled = true;
    else
    {
      irq_enabled = false;
      clear_pending_irq();
    }
  }
}

int M004(Cartriadge *cart, int addr)
{
  if (addr < 0x8000)
    return addr - 0x6000;

  int total = cart->pg_rom_size / 0x2000;
  int bank;
  if (addr < 0xA000)
    bank = prg_mode ? (total - 2) : banks[6];
  else if (addr < 0xC000)
    bank = banks[7];
  else if (addr < 0xE000)
    bank = prg_mode ? banks[6] : (total - 2);
  else
    bank = total - 1;

  return ((bank * 0x2000) + (addr & 0x1FFF)) % cart->pg_rom_size;
}

unsigned char M004_PPU(Cartriadge *cart, int addr)
{
  unsigned char *chr = cart->chr_ram ? cart->chr_ram : cart->ch_rom;
  int max_1k = cart->chr_ram ? (cart->ch_ram_size / 0x400)
                             : (cart->ch_rom_bank_count * 8);

  int bank, offset;
  if (chr_mode == 0)
  {
    if (addr < 0x0800)      { bank = banks[0] & 0xFE; offset = addr; }
    else if (addr < 0x1000) { bank = banks[1] & 0xFE; offset = addr - 0x0800; }
    else if (addr < 0x1400) { bank = banks[2]; offset = addr - 0x1000; }
    else if (addr < 0x1800) { bank = banks[3]; offset = addr - 0x1400; }
    else if (addr < 0x1C00) { bank = banks[4]; offset = addr - 0x1800; }
    else                    { bank = banks[5]; offset = addr - 0x1C00; }
  }
  else
  {
    if (addr < 0x0400)      { bank = banks[2]; offset = addr; }
    else if (addr < 0x0800) { bank = banks[3]; offset = addr - 0x0400; }
    else if (addr < 0x0C00) { bank = banks[4]; offset = addr - 0x0800; }
    else if (addr < 0x1000) { bank = banks[5]; offset = addr - 0x0C00; }
    else if (addr < 0x1800) { bank = banks[0] & 0xFE; offset = addr - 0x1000; }
    else                    { bank = banks[1] & 0xFE; offset = addr - 0x1800; }
  }

  return chr[((bank % max_1k) * 0x400 + offset)];
}

void M004_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value)
{
  if (!cart->chr_ram) return;
  int max_1k = cart->ch_ram_size / 0x400;

  int bank, offset;
  if (chr_mode == 0)
  {
    if (addr < 0x0800)      { bank = banks[0] & 0xFE; offset = addr; }
    else if (addr < 0x1000) { bank = banks[1] & 0xFE; offset = addr - 0x0800; }
    else if (addr < 0x1400) { bank = banks[2]; offset = addr - 0x1000; }
    else if (addr < 0x1800) { bank = banks[3]; offset = addr - 0x1400; }
    else if (addr < 0x1C00) { bank = banks[4]; offset = addr - 0x1800; }
    else                    { bank = banks[5]; offset = addr - 0x1C00; }
  }
  else
  {
    if (addr < 0x0400)      { bank = banks[2]; offset = addr; }
    else if (addr < 0x0800) { bank = banks[3]; offset = addr - 0x0400; }
    else if (addr < 0x0C00) { bank = banks[4]; offset = addr - 0x0800; }
    else if (addr < 0x1000) { bank = banks[5]; offset = addr - 0x0C00; }
    else if (addr < 0x1800) { bank = banks[0] & 0xFE; offset = addr - 0x1000; }
    else                    { bank = banks[1] & 0xFE; offset = addr - 0x1800; }
  }

  cart->chr_ram[(bank % max_1k) * 0x400 + offset] = value;
}

void M004_ScanlineTick(Cartriadge *cart)
{
  (void)cart;
  if (irq_counter == 0 || irq_reload)
  {
    irq_counter = irq_latch;
    irq_reload = false;
  }
  else
  {
    irq_counter--;
  }

  if (irq_counter == 0 && irq_enabled)
    trigger_irq();
}

#ifndef CARTRIADGE_H
#define CARTRIADGE_H

#include <stdbool.h>

typedef struct Cartriadge {
  unsigned char *pg_rom;
  unsigned char *ch_rom;

  unsigned char *chr_ram;
  unsigned char *prg_ram;

    unsigned char (*cpu_read)(struct Cartriadge *, int);
  unsigned char (*ppu_read)(struct Cartriadge *, int);
  void (*ppu_write)(struct Cartriadge *, int, unsigned char);
  void (*cart_writer)(struct Cartriadge *, int, unsigned char);
  void (*scanline_tick)(struct Cartriadge *);
  int size;
  int pg_rom_size;
  int ch_ram_size;
  int prg_ram_size;
  int mirroring_mode;

  int pg_rom_bank_size;
  int pg_rom_bank_count;

  int ch_rom_bank_size;
  int ch_rom_bank_count;

  char board_type[24];
  bool has_battery;

} Cartriadge;

#endif

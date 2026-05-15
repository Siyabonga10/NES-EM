#ifndef INES_ONE_ROM_INFO
#define INES_ONE_ROM_INFO
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
  size_t file_size;
  size_t no_of_pg_rom_banks;
  size_t no_of_ch_rom_banks;

  unsigned char flags6;
  unsigned char flags7;

  bool   has_pg_ram;
  size_t pg_ram_size;

  bool   has_ch_ram;
  size_t ch_ram_size;

  /* DB-derived (populated from GameDbEntry if found) */
  char   board_type[24];
  bool   has_battery;
  bool   from_database;

} iNesOneRomInfo;

#endif

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

  // NOTE: The ones listed below arent yet being used anywhere or accounted for
  bool has_pg_ram;
  size_t pg_ram_size;
  
  bool has_ch_ram;
  size_t ch_ram_size;

} iNesOneRomInfo;

#endif
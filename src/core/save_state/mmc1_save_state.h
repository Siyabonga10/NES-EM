#ifndef MMC1_SAVE_STATE_H
#define MMC1_SAVE_STATE_H
#include <stdbool.h>

typedef struct {
  unsigned char load;
  int           last_write_cycle;
  unsigned char control;
  unsigned char chr_bank_0;
  unsigned char chr_bank_1;
  unsigned char prg_bank;
  bool          prg_ram_e000_ok;
  bool          prg_ram_a000_ok;
  bool          is_snrom;
  bool          is_surom;
  int           outer_prg_bank;
  unsigned char prg_ram[0x2000];
  unsigned char chr_ram[0x2000];
} Mmc1SaveState;

#endif

#ifndef MMC3_SAVE_STATE_H
#define MMC3_SAVE_STATE_H
#include <stdbool.h>
#include "save_state_packed.h"

typedef PACKED_STRUCT {
  unsigned char bank_select;
  unsigned char banks[8];
  bool          chr_mode;
  bool          prg_mode;
  unsigned char irq_latch;
  unsigned char irq_counter;
  bool          irq_enabled;
  bool          irq_reload;
  int           mirroring_mode;
  unsigned char prg_ram[0x2000];
  unsigned char chr_ram[0x2000];
} Mmc3SaveState;

#endif

#ifndef FME7_SAVE_STATE_H
#define FME7_SAVE_STATE_H
#include <stdint.h>

typedef struct {
  unsigned char reg_index;
  unsigned char regs[16];
  uint16_t      irq_counter;
  unsigned char prg_ram[0x2000];
  unsigned char chr_ram[0x2000];
} Fme7SaveState;

#endif

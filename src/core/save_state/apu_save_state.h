#ifndef APU_SAVE_STATE_H
#define APU_SAVE_STATE_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  unsigned char  registers[16];
  unsigned char  dmc_regs[4];
  bool           channel_enable[5];
  unsigned char  length_counter[3];
  double         phase[3];
  unsigned short dmc_sample_addr;
  unsigned short dmc_sample_len;
  unsigned short dmc_bytes_remaining;
  unsigned char  dmc_shift_register;
  unsigned char  dmc_bits_remaining;
  unsigned char  dmc_output_level;
  bool           dmc_silence;
  double         dmc_cycle_accum;
} ApuSaveState;

#endif

#ifndef APU_SAVE_STATE_H
#define APU_SAVE_STATE_H
#include <stdbool.h>
#include "save_state_packed.h"
#include <stdint.h>
#include "save_state_packed.h"

typedef PACKED_STRUCT {
  unsigned char  registers[16];
  unsigned char  dmc_regs[4];
  bool           channel_enable[5];
  unsigned char  length_counter[4];
  double         phase[3];
  unsigned short dmc_sample_addr;
  unsigned short dmc_sample_len;
  unsigned short dmc_bytes_remaining;
  unsigned char  dmc_shift_register;
  unsigned char  dmc_bits_remaining;
  unsigned char  dmc_output_level;
  bool           dmc_silence;
  double         dmc_cycle_accum;
  int            last_cycles;

  bool           envelope_start[3];
  bool           envelope_loop[3];
  bool           envelope_constant[3];
  unsigned char  envelope_divider[3];
  unsigned char  envelope_decay[3];
  unsigned char  envelope_volume[3];

  bool           sweep_enabled[2];
  bool           sweep_negate[2];
  unsigned char  sweep_shift[2];
  unsigned char  sweep_divider[2];
  unsigned char  sweep_period[2];
  bool           sweep_reload[2];
  bool           sweep_mute[2];

  unsigned short pulse_timer[2];

  bool           linear_control;
  unsigned char  linear_reload_val;
  unsigned char  linear_counter;
  bool           linear_reload;

  unsigned short noise_lfsr;
  bool           noise_mode;
  double         noise_cycle_accum;

  unsigned int   frame_cycle_accum;
  unsigned char  frame_step;
  bool           frame_5step;
  bool           frame_irq_inhibit;
  bool           frame_irq;

  bool           dmc_irq_enable;
  bool           dmc_irq_flag;

  float          hp90_prev_out;
  float          hp90_prev_in;
  float          hp440_prev_out;
  float          hp440_prev_in;
  float          lp14k_prev_out;
} ApuSaveState;

#endif

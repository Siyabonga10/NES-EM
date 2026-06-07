#ifndef CPU_SAVE_STATE_H
#define CPU_SAVE_STATE_H
#include <stdbool.h>
#include "save_state_packed.h"

typedef PACKED_STRUCT {
  unsigned char a;
  unsigned char y;
  unsigned char x;
  unsigned char sp;
  unsigned char status;
  int           pc;
  unsigned char wram[0x800];
  int           remaining_clock_cycles;
  unsigned int  elapsed_clock_cycles;
  bool          can_execute_next_instruction;
  bool          pending_instr_valid;
  int           pending_original_cycles;
  unsigned char pending_opcode;
} CpuSaveState;

#endif

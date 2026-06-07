#ifndef UXROM_SAVE_STATE_H
#define UXROM_SAVE_STATE_H

typedef PACKED_STRUCT {
  unsigned char prg_bank;
  unsigned char prg_ram[0x2000];
} UxromSaveState;

#endif

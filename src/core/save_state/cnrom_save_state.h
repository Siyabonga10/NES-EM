#ifndef CNROM_SAVE_STATE_H
#define CNROM_SAVE_STATE_H

typedef struct {
  unsigned char chr_bank;
  unsigned char prg_ram[0x2000];
} CnromSaveState;

#endif

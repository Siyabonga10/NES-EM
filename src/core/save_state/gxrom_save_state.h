#ifndef GXROM_SAVE_STATE_H
#define GXROM_SAVE_STATE_H

typedef struct {
  unsigned char prg_bank;
  unsigned char chr_bank;
  unsigned char prg_ram[0x2000];
  unsigned char chr_ram[0x2000];
} GxromSaveState;

#endif

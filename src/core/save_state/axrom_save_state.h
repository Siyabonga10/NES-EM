#ifndef AXROM_SAVE_STATE_H
#define AXROM_SAVE_STATE_H

typedef PACKED_STRUCT {
  unsigned char prg_bank;
  int           mirroring_mode;
  unsigned char prg_ram[0x2000];
  unsigned char chr_ram[0x2000];
} AxromSaveState;

#endif

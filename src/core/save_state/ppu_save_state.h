#ifndef PPU_SAVE_STATE_H
#define PPU_SAVE_STATE_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int        current_dot;
  int        current_row;
  int        cycle_count;
  int        dma_cycles_remaining;
  bool       dma_active;
  uint16_t   registers[9];
  unsigned char vram[0x800];
  unsigned char palette_ram[32];
  uint16_t   internal_registers[4];
  unsigned char read_buffer;
  unsigned char oam[256];
  bool       sprite0_hit;
} PpuSaveState;

#endif

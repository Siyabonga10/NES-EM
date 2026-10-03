#ifndef PPU_H
#define PPU_H
#include "frameData.h"
#include "nesColor.h"

unsigned char read_ppu(int addr);
void          write_ppu(int addr, unsigned char byte);
void          boot_ppu(unsigned bpc_r, unsigned bpc_g, unsigned bpc_b, unsigned bpc_a);
void          kill_ppu();
FrameData    *request_frame();

unsigned char  read_ppu_vram(int addr);
const NesColor *get_system_palette(void);
unsigned char  read_palette_ram(int index);

#endif

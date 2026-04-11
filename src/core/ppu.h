#ifndef PPU_H
#define PPU_H
#include "frameData.h"

unsigned char readPPU(int addr);
void writePPU(int addr, unsigned char byte);
void bootPPU();
void killPPU();
FrameData *requestFrame();
void draw_nametable_dbg();

#endif
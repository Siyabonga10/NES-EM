#ifndef PPU_H
#define PPU_H

unsigned char readPPU(int addr);
void writePPU(int addr, unsigned char byte);
void bootPPU();
void killPPU();

#endif
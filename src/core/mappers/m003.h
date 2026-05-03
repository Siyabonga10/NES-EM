#ifndef M003_H
#define M003_H
#include "../cartriadge.h"

int M003(Cartriadge *cart, int addr);
unsigned char M003_PPU(Cartriadge *cart, int addr);
void M003_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);
void M003_Write(Cartriadge *cart, int addr, unsigned char value);

#endif
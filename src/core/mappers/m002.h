#ifndef M002_H
#define M002_H
#include "../cartriadge.h"

int M002(Cartriadge *cart, int addr);
unsigned char M002_PPU(Cartriadge *cart, int addr);
void M002_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);
void M002_Write(Cartriadge *cart, int addr, unsigned char value);

#endif
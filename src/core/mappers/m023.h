#ifndef M023_H
#define M023_H
#include "../cartriadge.h"

int M023(Cartriadge *cart, int addr);
int M023_PPU(Cartriadge *cart, int addr);
void M023_Write(Cartriadge *cart, int addr, unsigned char value);

#endif

#ifndef M005_H
#define M005_H
#include "../cartriadge.h"

int M005(Cartriadge *cart, int addr);
int M005_PPU(Cartriadge *cart, int addr);
void M005_Write(Cartriadge *cart, int addr, unsigned char value);

#endif
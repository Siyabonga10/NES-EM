#ifndef M004_H
#define M004_H
#include "../cartriadge.h"

int M004(Cartriadge *cart, int addr);
int M004_PPU(Cartriadge *cart, int addr);
void M004_Write(Cartriadge *cart, int addr, unsigned char value);

#endif
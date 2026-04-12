#ifndef M034_H
#define M034_H
#include "../cartriadge.h"

int M034(Cartriadge *cart, int addr);
int M034_PPU(Cartriadge *cart, int addr);
void M034_Write(Cartriadge *cart, int addr, unsigned char value);

#endif
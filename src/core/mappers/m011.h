#ifndef M011_H
#define M011_H
#include "../cartriadge.h"

int M011(Cartriadge *cart, int addr);
int M011_PPU(Cartriadge *cart, int addr);
void M011_Write(Cartriadge *cart, int addr, unsigned char value);

#endif
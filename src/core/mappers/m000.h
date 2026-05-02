#ifndef M000_H
#define M000_H
#include "../cartriadge.h"

int M000(Cartriadge *cart, int addr);
unsigned char M000_PPU(Cartriadge *cart, int addr);
void NO_WRITE(Cartriadge *cart, int addr, unsigned char value);
void M000_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);

#endif
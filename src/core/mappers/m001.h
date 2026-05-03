#ifndef M001_H
#define M001_H
#include "../cartriadge.h"

int M001(Cartriadge *cart, int addr);
unsigned char M001_PPU(Cartriadge *cart, int addr);
void M001_Write(Cartriadge *cart, int addr, unsigned char value);
void M001_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);

#endif
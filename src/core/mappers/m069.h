#ifndef M069_H
#define M069_H
#include "../cartriadge.h"

int M069(Cartriadge *cart, int addr);
unsigned char M069_PPU(Cartriadge *cart, int addr);
void M069_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);
void M069_Write(Cartriadge *cart, int addr, unsigned char value);

#endif

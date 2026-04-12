#ifndef M007_H
#define M007_H
#include "../cartriadge.h"

int M007(Cartriadge *cart, int addr);
int M007_PPU(Cartriadge *cart, int addr);
void M007_Write(Cartriadge *cart, int addr, unsigned char value);

#endif
#ifndef M006_H
#define M006_H
#include "../cartriadge.h"

int M006(Cartriadge *cart, int addr);
int M006_PPU(Cartriadge *cart, int addr);
void M006_Write(Cartriadge *cart, int addr, unsigned char value);

#endif
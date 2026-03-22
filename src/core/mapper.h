#ifndef MAPPER_H
#define MAPPER_H
#include "cartriadge.h"

int M000(Cartriadge *cart, int addr);
int M000_PPU(Cartriadge *cart, int addr);
int M001(Cartriadge *cart, int addr);
int M001_PPU(Cartriadge *cart, int addr);

void M001_Write(Cartriadge *cart, int addr, unsigned char value);
void NO_WRITE(Cartriadge *cart, int addr, unsigned char value);

#endif
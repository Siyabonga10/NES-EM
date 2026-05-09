#ifndef M001_H
#define M001_H
#include "../cartriadge.h"
#include "../ines_one_rom_info.h"

int M001(Cartriadge *cart, int addr);
unsigned char M001_PPU(Cartriadge *cart, int addr);
void M001_Write(Cartriadge *cart, int addr, unsigned char value);
void M001_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);

void mount_mapper_002_to_cartridge(Cartriadge* cart, iNesOneRomInfo cart_info);

#endif
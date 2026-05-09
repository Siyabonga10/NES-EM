#ifndef M000_H
#define M000_H
#include "../cartriadge.h"
#include "../ines_one_rom_info.h"

int M000(Cartriadge *cart, int addr);
unsigned char M000_PPU(Cartriadge *cart, int addr);
void NO_WRITE(Cartriadge *cart, int addr, unsigned char value);
void M000_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);

void mount_mapper_001_to_cartridge(Cartriadge* cart, iNesOneRomInfo cart_info);

#endif
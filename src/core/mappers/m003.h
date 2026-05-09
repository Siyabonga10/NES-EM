#ifndef M003_H
#define M003_H
#include "../cartriadge.h"
#include "../ines_one_rom_info.h"

int M003(Cartriadge *cart, int addr);
unsigned char M003_PPU(Cartriadge *cart, int addr);
void M003_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);
void M003_Write(Cartriadge *cart, int addr, unsigned char value);

void mount_mapper_003_to_cartridge(Cartriadge* cart, iNesOneRomInfo cart_info);

#endif
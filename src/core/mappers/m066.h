#ifndef M066_H
#define M066_H
#include "../cartriadge.h"
#include "../ines_one_rom_info.h"

int M066(Cartriadge *cart, int addr);
unsigned char M066_PPU(Cartriadge *cart, int addr);
void M066_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);
void M066_Write(Cartriadge *cart, int addr, unsigned char value);

void mount_mapper_006_to_cartridge(Cartriadge* cart, iNesOneRomInfo cart_info);

#endif

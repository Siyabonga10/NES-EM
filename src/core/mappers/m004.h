#ifndef M004_H
#define M004_H
#include "../cartriadge.h"
#include "../ines_one_rom_info.h"

int M004(Cartriadge *cart, int addr);
unsigned char M004_PPU(Cartriadge *cart, int addr);
void M004_PPU_WRITE(Cartriadge *cart, int addr, unsigned char value);
void M004_Write(Cartriadge *cart, int addr, unsigned char value);
void M004_ScanlineTick(Cartriadge *cart);

void mount_mapper_004_to_cartridge(Cartriadge* cart, iNesOneRomInfo cart_info);

#endif

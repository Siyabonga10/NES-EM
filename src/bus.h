#ifndef BUS_H
#define BUS_H
#include <stdbool.h>

unsigned char readByte(int addr);
void writeByte(int addr, unsigned char value);


int getCPUStatusFlag(int position);
void setCPUStatusFlag(int position, bool value);

#endif BUS_H
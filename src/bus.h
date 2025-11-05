#ifndef BUS_H
#define BUS_H
#include <stdbool.h>

unsigned char readByte(int addr);
void writeByte(int addr, unsigned char value);


int getCPUStatusFlag(int position);
void setCPUStatusFlag(int position, bool value);
int getPC();
void setPC(int newPC);

// return addresses to said registers
int getCPU_Stack();
int getCPU_XRegister();
int getCPU_YRegister();
int getCPU_Accumulator();
int getCPU_StatusRegister();

#endif 
#include "bus.h"

unsigned char readByte(int addr) {return 0;}
void writeByte(int addr, unsigned char value) {}


int getCPUStatusFlag(int position) {return -1;}
void setCPUStatusFlag(int position, bool value) {}
int getPC() {return 0;}
void setPC(int newPC) {};

// return addresses to said registers
int getCPU_Stack() {}
int getCPU_XRegister() {}
int getCPU_YRegister() {}
int getCPU_Accumulator() {}
int getCPU_StatusRegister() {}

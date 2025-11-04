#ifndef ADDRESSING_MODES_H
#define ADDRESSING_MODES_H
#include "cpu.h"
#include "bus.h"
// All addressing modes return a pointer to the operand for an instruction
// since 0 can be an address, we use -1 for an invalid address
// Some of the addressing modes do not use the program counter passed in, but for consistency (same function pointers), all signatures are the same

int ABS_A(int PC);
int ABS_INDX_IND(int PC);
int ABS_INDEX_X(int PC);
int ABS_INDEX_Y(int PC);
int ABS_IND(int PC);
int ACC(int PC);
int IMM(int PC);
int IMP(int PC);
int PCR(int PC);
int STK(int PC);
int ZP(int PC);
int ZP_INDX_IND(int PC);
int ZP_INDX_X(int PC);
int ZP_INDX_Y(int PC);
int ZP_IND(int PC);
int ZP_IND_INDX_Y(int PC);

#endif ADDRESSING_MODES_H
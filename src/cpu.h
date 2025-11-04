#ifndef CPU_H
#define CPU_H
#include <stdbool.h>
#include <stdlib.h>
#include "ExecutionInfo.h"

int PC = 0xFFFC; // starting point of execution 
static const int NO_OF_REGISTERS = 5;
static const int WRAM_SIZE = 0x800;
static unsigned char* cpuMem;
/*
The registers are stored as part of the WRAM for convinience, the order is
Accumlator
Y Register
X Register
Stack
Status register
*/

// TODO: Move these further up, past the 64kB range, to just avoid headaches
static const int ACCUMULATOR_ADDR = 0;
static const int Y_REGISTER_ADDR = 1;
static const int X_REGISTER_ADDR = 2;
static const int STACK_ADDR = 3;
static const int STATUS_REGISTER_ADDR = 4;
bool running = true;

void boot();
void run();
void shutdown();

void executeInstruction(ExecutionInfo exInfo);


int getCPU_Stack();
int getCPU_XRegister();
int getCPU_YRegister();
int getCPU_Accumulator();
int getCPU_StatusRegister();

#endif CPU_H
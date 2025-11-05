#include "cpu.h"
#include "instructions.h"
#include "bus.h"
#include <string.h>

static int PC = 0xFFFC; // starting point of execution 
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
static const int REGISTER_OFFSET = 0xFFFFFF;
static const int ACCUMULATOR_ADDR = 0;
static const int Y_REGISTER_ADDR = 1;
static const int X_REGISTER_ADDR = 2;
static const int STACK_ADDR = 3;
static const int STATUS_REGISTER_ADDR = 4;
static bool running = true;

void bootCPU()
{
    cpuMem = (unsigned char*)malloc(WRAM_SIZE + NO_OF_REGISTERS);
    memset(cpuMem, 0, WRAM_SIZE + NO_OF_REGISTERS);
    PC = readByte(PC) + readByte(PC + 1) << 8; // Get the starting address for execution
}

void runCPU()
{
    while(running) {
        ExecutionInfo nextIntruction = getExecutionInfo(PC);
        PC += 1;
        executeInstruction(nextIntruction);
    }
}

void shutdownCPU()
{
    free(cpuMem);
}

void executeInstruction(ExecutionInfo exInfo)
{
    int operandAddr = exInfo.addressingMode(PC);
    exInfo.executor(operandAddr);
    PC += exInfo.instructionSize - 1; // Subtract one for the op code, that has already been accounted for
}


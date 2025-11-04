#include "cpu.h"
#include "instructions.h"
#include "bus.h"
#include <string.h>

void boot()
{
    cpuMem = malloc(WRAM_SIZE + NO_OF_REGISTERS);
    memset(cpuMem, 0, WRAM_SIZE + NO_OF_REGISTERS);
    PC = readByte(PC) + readByte(PC + 1) << 8; // Get the starting address for execution
}

void run()
{
    while(running) {
        ExecutionInfo nextIntruction = getExecutionInfo(PC);
        PC += 1;
        executeInstruction(nextIntruction);
    }
}

void shutdown()
{
    free(cpuMem);
}

void executeInstruction(ExecutionInfo exInfo)
{
    int operandAddr = exInfo.addressingMode(PC);
    exInfo.executor(operandAddr);
    PC += exInfo.instructionSize - 1; // Subtract one for the op code, that has already been accounted for
}

int getCPU_Stack() {return STACK_ADDR;}
int getCPU_XRegister() {return X_REGISTER_ADDR;}
int getCPU_YRegister() {return Y_REGISTER_ADDR;}
int getCPU_Accumulator() {return ACCUMULATOR_ADDR;}
int getCPU_StatusRegister() {return STATUS_REGISTER_ADDR;}

#include "cpu.h"
#include "instructions.h"
#include "bus.h"
#include "registerOffsets.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

static int PC = 0xFFFC; // starting point of execution 
static const int NO_OF_REGISTERS = 5;
static const int WRAM_SIZE = 0x800;
static unsigned char* cpuMem;

static bool running = true;

void runCPU()
{
    while(running) {
        ExecutionInfo nextIntruction = getExecutionInfo(PC);
        PC += 1;
        executeInstruction(nextIntruction);
        printf("PC at %i\n", PC);
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

static int statusFlagGetter(int index) {
    assert(index < 8);
    unsigned char status_register = cpuMem[STATUS_REGISTER_ADDR];
    return status_register & (1 << index);
}

static void statusFlagSetter(int index, bool value) {
    assert(index < 8);
    if(value) 
        cpuMem[STATUS_REGISTER_ADDR] |= (1 << index);
    else
        cpuMem[STATUS_REGISTER_ADDR] &= ~(1 << index);
}
static int pcGetter() {
    return PC;
}
static void pcSetter(int newPC) {
    PC = newPC;
}
static void stackPush(unsigned char byte) {
    cpuMem[0x100 + cpuMem[STACK_ADDR]] = byte;
    cpuMem[STACK_ADDR] -= 1;
}
static unsigned char stackPop() {
    unsigned char byte = cpuMem[0x100 + cpuMem[STACK_ADDR]];
    cpuMem[STACK_ADDR] += 1;
    return byte;
}

unsigned char readCPU(int addr) {
    assert(addr < 0x2000);
    return cpuMem[addr];
}

void writeCPU(int addr, unsigned char value) {
    if(addr < 0x2000)
        cpuMem[addr] = value;   
}

void bootCPU()
{
    printf("Booting CPU\n");
    cpuMem = (unsigned char*)malloc(WRAM_SIZE + NO_OF_REGISTERS);
    memset(cpuMem, 0, WRAM_SIZE + NO_OF_REGISTERS);
    PC = readByte(PC) + readByte(PC + 1) << 8; // Get the starting address for execution
    cpuMem[STACK_ADDR] = 0xFF;
    connectCPUToBus(statusFlagGetter, statusFlagSetter, pcGetter, pcSetter, stackPush, stackPop, readCPU, writeCPU);
    printf("Boot complete\n");
}
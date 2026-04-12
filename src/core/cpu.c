#include "cpu.h"
#include "instructions.h"
#include "bus.h"
#include "ppu.h"
#include "registerOffsets.h"
#include "addressingModes.h"
#include "ControllerKeyStates.h"
#include "frameData.h"
#include "controller.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define PPU_TICKS_PER_CPU_CYCLE 3

static int PC = 0xFFFC; // starting point of execution
static const int WRAM_SIZE = 0x800;
static unsigned char *cpuMem;

static bool canExecuteNextInstruction = false;
static int remainingClockCycles = 0;

void doSingleTickAndCheckForNMI(ControllerKeyStates *keyStates)
{
    ppu_tick();
    if (pendingNMI())
    {
        updateControllerInput(keyStates);
    }
}

void tickPPU(ControllerKeyStates *keyStates)
{

    for (int i = 0; i < PPU_TICKS_PER_CPU_CYCLE; i++)
        doSingleTickAndCheckForNMI(keyStates);
}
static int someCounter;
FrameData *tickCPU(ControllerKeyStates *keyStates)
{
    while (true)
    {
        FrameData *frame = requestFrame();
        if (frame->is_new_frame)
            return frame;
        updateControllerInput(keyStates);
        
        if (is_dma_active())
        {
            update_dma_cycles();
            tickPPU(keyStates);
            continue;
        }
        
        if (canExecuteNextInstruction && pendingNMI())
        {
            updateControllerInput(keyStates);
            executeNMI();
            continue;
        }
        
        if (canExecuteNextInstruction)
        {
            ExecutionInfo instr = getNextInstruction();
            remainingClockCycles = executeInstruction(instr) - 1;
            cpu_instruction_completed();
            canExecuteNextInstruction = remainingClockCycles == 0;
            tickPPU(keyStates);
        }
        else
        {
            remainingClockCycles--;
            if (remainingClockCycles < 0)
            {
                canExecuteNextInstruction = true;
            }
            else
            {
                tickPPU(keyStates);
            }
        }
    }
}

ExecutionInfo getNextInstruction()
{
    ExecutionInfo nextIntruction = getExecutionInfo(readByte(PC));
    PC += 1;
    return nextIntruction;
}

void shutdownCPU()
{
    free(cpuMem);
}

int executeInstruction(ExecutionInfo exInfo)
{
    int operandAddr = exInfo.addressingMode(PC);
    int additionalCycles = 0;
    exInfo.executor(operandAddr, &additionalCycles);
    PC += exInfo.instructionSize - 1; // Subtract one for the op code, that has already been accounted for
    return exInfo.clockCycles + additionalCycles;
}

static int statusFlagGetter(int index)
{
    assert(index < 8);
    unsigned char status_register = cpuMem[STATUS_REGISTER_ADDR];
    bool val = (status_register & (1 << index)) ? 1 : 0;
    return val;
}

static void statusFlagSetter(int index, bool value)
{
    assert(index < 8);
    if (value)
        cpuMem[STATUS_REGISTER_ADDR] |= (1 << index);
    else
        cpuMem[STATUS_REGISTER_ADDR] &= ~(1 << index);
}
static int pcGetter()
{
    return PC;
}
static void pcSetter(int newPC)
{
    PC = newPC;
}
static void stackPush(unsigned char byte)
{
    cpuMem[NO_OF_REGISTERS + 0x100 + cpuMem[STACK_ADDR]] = byte;
    cpuMem[STACK_ADDR] -= 1;
}
static unsigned char stackPop()
{
    cpuMem[STACK_ADDR] += 1;
    unsigned char byte = cpuMem[NO_OF_REGISTERS + 0x100 + cpuMem[STACK_ADDR]];
    return byte;
}

unsigned char readCPU(int addr)
{
    if (addr < 0x2000)
        return cpuMem[NO_OF_REGISTERS + (addr % 0x800)];
    else if (addr >= REGISTER_OFFSET)
        return cpuMem[addr - REGISTER_OFFSET];
    return 0;
}

void writeCPU(int addr, unsigned char value)
{
    if (addr < 0x2000)
        cpuMem[NO_OF_REGISTERS + (addr % 0x800)] = value;
    else if (addr >= REGISTER_OFFSET)
        cpuMem[addr - REGISTER_OFFSET] = value;
}
void bootCPU()
{
    printf("Booting CPU\n");

    PC = 0xFFFC;
    cpuMem = (unsigned char *)malloc(WRAM_SIZE + NO_OF_REGISTERS);
    memset(cpuMem, 0, WRAM_SIZE + NO_OF_REGISTERS);
    PC = readByte(PC) + ((int)readByte(PC + 1) << 8); // Get the starting address for execution
    cpuMem[STACK_ADDR] = 0xFF;
    connectCPUToBus(statusFlagGetter, statusFlagSetter, pcGetter, pcSetter, stackPush, stackPop, readCPU, writeCPU, NMI);
    canExecuteNextInstruction = true;

    printf("Boot complete\n");
}

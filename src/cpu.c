#include "cpu.h"
#include "instructions.h"
#include "bus.h"
#include "registerOffsets.h"
#include "addressingModes.h"
#include "controller.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <raylib.h>
#define PPU_TICKS_PER_CPU_CYCLE 3

static int PC = 0xFFFC; // starting point of execution
static const int WRAM_SIZE = 0x800;
static unsigned char *cpuMem;
static int baseWidth = 256;
static int baseHeight = 240;
static float scallingF = 3.0f;

static bool running = true;
static bool canExecuteNextInstruction = false;
static int remainingClockCycles = 0;
static int totalClockCycles = 0;

static bool dumped = false;
static bool killed = false;

static void renderDiagnostics();

void doSingleTickAndCheckForNMI()
{
    ppu_tick();
    if (pendingNMI())
    {
        updateControllerInput();
        executeNMI();
    }
}

void tickPPU()
{

    for (int i = 0; i < PPU_TICKS_PER_CPU_CYCLE; i++)
        doSingleTickAndCheckForNMI();
}

void handleInput()
{
    if (IsKeyPressed(KEY_D) && !dumped)
    {
        dump6004();
        dumped = true;
    }
    if (IsKeyPressed(KEY_Q))
    {
        shutdownCPU();
    }
}

static bool maybe_exiting_delay = false;
static bool was_last_instruction_pla = false;
void runCPU()
{
    while (!WindowShouldClose())
    {
        handleInput();

        if (canExecuteNextInstruction)
        {
            ExecutionInfo instr = getNextInstruction();
            remainingClockCycles = executeInstruction(instr) - 1;
            canExecuteNextInstruction = remainingClockCycles == 0;
            tickPPU();
            if (instr.executor == PLP && was_last_instruction_pla)
            {
                maybe_exiting_delay = true;
            }
            else
            {
                maybe_exiting_delay = false;
            }
            was_last_instruction_pla = instr.executor == PLA;
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
                tickPPU();
            }
        }

        totalClockCycles++;
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
    if (killed)
        return;
    killed = true;
    CloseWindow();
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
    killed = false;
    PC = 0xFFFC;
    printf("Booting CPU\n");

    cpuMem = (unsigned char *)malloc(WRAM_SIZE + NO_OF_REGISTERS);
    memset(cpuMem, 0, WRAM_SIZE + NO_OF_REGISTERS);
    PC = readByte(PC) + ((int)readByte(PC + 1) << 8); // Get the starting address for execution
    cpuMem[STACK_ADDR] = 0xFF;
    connectCPUToBus(statusFlagGetter, statusFlagSetter, pcGetter, pcSetter, stackPush, stackPop, readCPU, writeCPU, NMI);
    printf("Boot complete\n");
    canExecuteNextInstruction = true;
}

void renderStatusRegister(int height)
{
    char letters[8] = {'C', 'Z', 'I', 'D', 'B', '1', 'V', 'N'};
    int status = cpuMem[STATUS_REGISTER_ADDR];
    for (int i = 0; i < 8; i++)
    {
        DrawText(TextFormat("%c", letters[i]), scallingF * baseWidth + i * 30, height, 20, WHITE);
        int bitValue = statusFlagGetter(i);
        DrawText(TextFormat("%i", bitValue), scallingF * baseWidth + i * 30, height + 30, 20, WHITE);
    }
}

static void renderLastFiveStackItems(int height)
{
    int sp = cpuMem[STACK_ADDR] + 1;
    int items = 0;
    while (sp <= 0xFF)
    {
        DrawText(TextFormat("%X: %X", sp, cpuMem[NO_OF_REGISTERS + 0x100 + sp]), scallingF * baseWidth, height, 20, WHITE);
        sp += 1;
        height += 20;
        items += 1;
        if (items > 10)
            break;
    }
}

static void renderNextPCItems(int height)
{
    for (int i = 0; i < 5; i++)
    {
        DrawText(TextFormat("%X: %X", PC + i, readByte(PC + i)), scallingF * baseWidth + 100, height, 20, WHITE);
        height += 20;
    }
}

static void renderRegion(int height, int start, int end)
{
    for (int i = start; i < end; i++)
    {
        DrawText(TextFormat("%X: %X", i, readByte(i)), scallingF * baseWidth + 100, height, 20, WHITE);
        height += 20;
    }
}

static void renderDiagnostics()
{
    int startingHeight = 20;
    int increment = 30;
    int rightOffset = 120;
    DrawText(TextFormat("PC: %X", PC), scallingF * baseWidth, startingHeight, 20, WHITE);
    DrawText(TextFormat("A: %X", cpuMem[ACCUMULATOR_ADDR]), scallingF * baseWidth + baseWidth - rightOffset, startingHeight, 20, WHITE);
    startingHeight += increment;
    DrawText(TextFormat("X: %X", cpuMem[X_REGISTER_ADDR]), scallingF * baseWidth, startingHeight, 20, WHITE);
    DrawText(TextFormat("Y: %X", cpuMem[Y_REGISTER_ADDR]), scallingF * baseWidth + baseWidth - rightOffset, startingHeight, 20, WHITE);
    startingHeight += increment;
    DrawText(TextFormat("status: %X", cpuMem[STATUS_REGISTER_ADDR]), scallingF * baseWidth, startingHeight, 20, WHITE);
    DrawText(TextFormat("stack: %X", cpuMem[STACK_ADDR]), scallingF * baseWidth + baseWidth - rightOffset, startingHeight, 20, WHITE);
    startingHeight += increment;
    renderStatusRegister(startingHeight);
    startingHeight += 50;
    renderLastFiveStackItems(startingHeight);
    renderNextPCItems(startingHeight);
}
#include "cpu.h"
#include "instructions.h"
#include "bus.h"
#include "registerOffsets.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <raylib.h>


static int PC = 0xFFFC; // starting point of execution 
static const int NO_OF_REGISTERS = 5;
static const int WRAM_SIZE = 0x800;
static unsigned char* cpuMem;
static int baseWidth = 256;
static int baseHeight = 240;
static float scallingF = 3.5;

static bool running = true;

static void renderDiagnostics();

void runCPU()
{
    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        renderDiagnostics();
        EndDrawing();   
        if(IsKeyPressed(KEY_SPACE)) {
            ExecutionInfo nextIntruction = getExecutionInfo(PC);
            PC += 1;
            executeInstruction(nextIntruction);
        }
    }
}

void shutdownCPU()
{
    CloseWindow();
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
    return status_register & (1 << index) ? 1 : 0;
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
    if(addr < 0x2000)
        return cpuMem[addr];
    else if(addr >= REGISTER_OFFSET)
        return cpuMem[addr - REGISTER_OFFSET];
}

void writeCPU(int addr, unsigned char value) {
    if(addr < 0x2000)
        cpuMem[addr] = value;
    else if(addr >= REGISTER_OFFSET)
        cpuMem[addr - REGISTER_OFFSET] = value;
}

void bootCPU()
{
    printf("Booting CPU\n");
    InitWindow(baseWidth * scallingF + baseWidth, baseHeight * scallingF, "NES emulator"); // last segment on the right used to render debug info
    cpuMem = (unsigned char*)malloc(WRAM_SIZE + NO_OF_REGISTERS);
    memset(cpuMem, 0, WRAM_SIZE + NO_OF_REGISTERS);
    PC = readByte(PC) + readByte(PC + 1) << 8; // Get the starting address for execution
    cpuMem[STACK_ADDR] = 0xFF;
    connectCPUToBus(statusFlagGetter, statusFlagSetter, pcGetter, pcSetter, stackPush, stackPop, readCPU, writeCPU);
    printf("Boot complete\n");
}

void renderStatusRegister(int height) {
    char letters[8] = {'C', 'Z', 'I', 'D', 'B', '1', 'V', 'N'};
    int status = cpuMem[STATUS_REGISTER_ADDR];
    for(int i  = 0; i < 8; i++) {
        DrawText(TextFormat("%c", letters[i]), scallingF * baseWidth + i * 30, height, 20, WHITE);
        int bitValue = statusFlagGetter(i);
        DrawText(TextFormat("%i", bitValue), scallingF * baseWidth + i * 30, height + 30, 20, WHITE);
    }
}

static void renderDiagnostics() {
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

}
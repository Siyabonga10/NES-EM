#include "bus.h"
#include "registerOffsets.h"
#include <stdlib.h>
#include <stdio.h>

unsigned char (*cpuReader)(int);
void (*cpuWritter)(int, unsigned char);
Cartriadge* cartriadge = NULL; 

unsigned char readByte(int addr) {
    if(addr < 0x2000)
        return cpuReader(addr);
    else if(0x6000 <= addr && addr < 0xFFFF && cartriadge != NULL)
        return cartriadge->mem[cartriadge->mapper(addr)];
    else if(addr >= REGISTER_OFFSET)
        return cpuReader(addr);
    return 0xFF;
}
void writeByte(int addr, unsigned char value) {
    if(addr < 0x2000)
        cpuWritter(addr, value);
    else if(0x6000 <= addr && addr < 0xFFFF)
        cartriadge->mem[cartriadge->mapper(addr)] = value;
    else if(addr >= REGISTER_OFFSET)
        cpuWritter(addr, value);
}

// return addresses to said registers
int getCPU_Stack() {return REGISTER_OFFSET + STACK_ADDR;}
int getCPU_XRegister() {return REGISTER_OFFSET + X_REGISTER_ADDR;}
int getCPU_YRegister() {return REGISTER_OFFSET + Y_REGISTER_ADDR;}
int getCPU_Accumulator() {return REGISTER_OFFSET + ACCUMULATOR_ADDR;}
int getCPU_StatusRegister() {return REGISTER_OFFSET + STATUS_REGISTER_ADDR;}

static int (*statusFlagGetter)(int);
static void (*statusFlagSetter)(int, bool);
static int (*pcGetter)();
static void (*pcSetter)(int);
static void (*stackPush)(unsigned char);
static unsigned char (*stackPop)();
void connectCPUToBus(int (*CPUstatusFlagGetter)(int), void (*CPUstatusFlagSetter)(int, bool), int (*CPUpcGetter)(), void (*CPUpcSetter)(int), void (*CPUstackPush)(unsigned char), unsigned char (*CPUstackPop)(), unsigned char (*readCPU)(), void (*writeCPU)(int, unsigned char)) {
    statusFlagGetter = CPUstatusFlagGetter;
    statusFlagSetter = CPUstatusFlagSetter;
    pcGetter = CPUpcGetter;
    pcSetter = CPUpcSetter;
    stackPush = CPUstackPush;
    stackPop = CPUstackPop;
    cpuReader = readCPU;
    cpuWritter = writeCPU;
}

// These call the methods on the CPU, use function pointers to avoid circular deps, maybe messy
int getCPUStatusFlag(int position) {return statusFlagGetter(position);}
void setCPUStatusFlag(int position, bool value) {statusFlagSetter(position, value);}
int getPC() {return pcGetter();}
void setPC(int newPC) {pcSetter(newPC);}
void pushToStack(unsigned char byte) {stackPush(byte);}
void dump6004()
{
    printf(cartriadge->mem + 0x6004);
    printf("\nDone\n");
}
unsigned char popFromStack() { return stackPop(); }

void connectCartriadgeToBus(Cartriadge *cart) {
    cartriadge = cart;
};
#include "bus.h"
#include "registerOffsets.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

unsigned char (*cpuReader)(int);
void (*cpuWritter)(int, unsigned char);
unsigned char (*ppuReader)(int);
void (*ppuWriter)(int, unsigned char);
Cartriadge *cartriadge = NULL;
static unsigned char (*controllerReader_)(int);
static void (*controllerWritter_)(int, unsigned char);

unsigned char readByte(int addr)
{
    if (addr < 0x2000)
        return cpuReader(addr);
    else if (0x2000 <= addr && addr < 0x4000)
        return ppuReader(addr);
    else if (addr >= 0x4000 && addr <= 0x4017)
        return controllerReader_(addr);
    else if (0x6000 <= addr && addr <= 0xFFFF && cartriadge != NULL)
        return cartriadge->mem[cartriadge->mapper(cartriadge, addr)];

    else if (addr >= REGISTER_OFFSET)
        return cpuReader(addr);
    return 0xFF;
}
void writeByte(int addr, unsigned char value)
{
    if (addr < 0x2000)
        cpuWritter(addr, value);
    else if (0x2000 <= addr && addr < 0x4000)
    {
        ppuWriter(addr, value);
    }
    else if (addr == 0x4014)
    {
        ppuWriter(addr, value);
    }
    else if (addr >= 0x4000 && addr <= 0x4017)
    {
        controllerWritter_(addr, value);
    }
    else if (0x8000 <= addr && addr <= 0xFFFF)
    {
        cartriadge->cartWriter(cartriadge, addr, value);
    }
    else if (0x6000 <= addr && addr < 0x8000)
    {
        cartriadge->mem[cartriadge->mapper(cartriadge, addr)] = value;
    }
    else if (addr >= REGISTER_OFFSET)
        cpuWritter(addr, value);
}

unsigned char readBytePPU(int addr)
{
    return cartriadge->mem[cartriadge->ppuMapper(cartriadge, addr)];
}

unsigned char fetchFromCPU(int addr)
{
    if (addr < 0x4000)
        return cpuReader(addr);
    return 0;
}

// return addresses to said registers
int getCPU_Stack() { return REGISTER_OFFSET + STACK_ADDR; }
int getCPU_XRegister() { return REGISTER_OFFSET + X_REGISTER_ADDR; }
int getCPU_YRegister() { return REGISTER_OFFSET + Y_REGISTER_ADDR; }
int getCPU_Accumulator() { return REGISTER_OFFSET + ACCUMULATOR_ADDR; }
int getCPU_StatusRegister() { return REGISTER_OFFSET + STATUS_REGISTER_ADDR; }

static int (*statusFlagGetter)(int);
static void (*statusFlagSetter)(int, bool);
static int (*pcGetter)();
static void (*pcSetter)(int);
static void (*stackPush)(unsigned char);
static unsigned char (*stackPop)();
static void (*nmiTrigger)();
void connectCPUToBus(int (*CPUstatusFlagGetter)(int),
                     void (*CPUstatusFlagSetter)(int, bool),
                     int (*CPUpcGetter)(), void (*CPUpcSetter)(int),
                     void (*CPUstackPush)(unsigned char),
                     unsigned char (*CPUstackPop)(),
                     unsigned char (*readCPU)(),
                     void (*writeCPU)(int, unsigned char),
                     void (*nmi_trigger)())
{
    statusFlagGetter = CPUstatusFlagGetter;
    statusFlagSetter = CPUstatusFlagSetter;
    pcGetter = CPUpcGetter;
    pcSetter = CPUpcSetter;
    stackPush = CPUstackPush;
    stackPop = CPUstackPop;
    cpuReader = readCPU;
    cpuWritter = writeCPU;
    nmiTrigger = nmi_trigger;
}

// These call the methods on the CPU, use function pointers to avoid circular deps, maybe messy
int getCPUStatusFlag(int position) { return statusFlagGetter(position); }
void setCPUStatusFlag(int position, bool value) { statusFlagSetter(position, value); }
int getPC() { return pcGetter(); }
void setPC(int newPC) { pcSetter(newPC); }
void pushToStack(unsigned char byte) { stackPush(byte); }
void dump6004()
{
    printf("%s", cartriadge->mem + 4);
}
void triggerNMI()
{
    nmiTrigger();
}
unsigned char popFromStack() { return stackPop(); }

void connectCartriadgeToBus(Cartriadge *cart)
{
    cartriadge = cart;
}

void connectController(unsigned char (*controllerReader)(int), void (*controllerWritter)(int, unsigned char))
{
    controllerReader_ = controllerReader;
    controllerWritter_ = controllerWritter;
};

static void (*tick_ppu)();
void connect_ppu_to_bus(void (*ppu_ticker)(), unsigned char (*ppu_reader)(int), void (*ppu_writer)(int, unsigned char))
{
    tick_ppu = ppu_ticker;
    ppuReader = ppu_reader;
    ppuWriter = ppu_writer;
}
void ppu_tick()
{
    tick_ppu();
}

Cartriadge *getCatriadge()
{
    return cartriadge;
}

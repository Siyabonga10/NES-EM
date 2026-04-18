#include "bus.h"
#include "registerOffsets.h"
#include "instructions.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define DEBUG_READ 0

unsigned char (*cpuReader)(int);
void (*cpuWritter)(int, unsigned char);
unsigned char (*ppuReader)(int);
void (*ppuWriter)(int, unsigned char);
unsigned char (*apuReader)(int);
void (*apuWriter)(int, unsigned char);
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
    else if (addr >= 0x4000 && addr <= 0x4013)
        apuWriter(addr, value);
    else if (addr == 0x4014)
        ppuWriter(addr, value);
    else if (addr > 0x4014 && addr < 0x4016)
        apuWriter(addr, value);
    else if (addr == 0x4016)
    {
        controllerWritter_(addr, value);
        apuWriter(addr, value);
    }

    else if (addr == 0x4017)
    {
        controllerWritter_(addr, value);
        apuWriter(addr, value);
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
#if DEBUG_READ
    static int read_count = 0;
    if (read_count < 10) {
        fprintf(stderr, "[READBYTEPPU %d] addr=0x%04X\n", read_count, addr);
        read_count++;
    }
#endif
    if (addr < 0x2000) {
        // CHR address space
        if (cartriadge->chr_ram != NULL) {
            // CHR-RAM with banking
            int offset = cartriadge->ppuMapper(cartriadge, addr);
#if DEBUG_READ
            if (read_count <= 10) {
                fprintf(stderr, "  -> CHR-RAM offset=0x%04X\n", offset);
            }
#endif
            if (cartriadge->ch_ram_size > 0) {
                offset %= cartriadge->ch_ram_size;
            }
            return cartriadge->chr_ram[offset];
        } else {
            // CHR-ROM
            int offset = cartriadge->ppuMapper(cartriadge, addr);
#if DEBUG_READ
            if (read_count <= 10) {
                fprintf(stderr, "  -> CHR-ROM offset=0x%04X\n", offset);
            }
#endif
            return cartriadge->mem[offset];
        }
    }
    // Pattern table reads beyond 0x2000 shouldn't happen, but fallback
    int offset = cartriadge->ppuMapper(cartriadge, addr);
#if DEBUG_READ
    if (read_count <= 10) {
        fprintf(stderr, "  -> fallback offset=0x%04X\n", offset);
    }
#endif
    return cartriadge->mem[offset];
}

unsigned char fetchFromCPU(int addr)
{
    // DMA reads: support full address space but skip PPU registers
    // to avoid side effects (reading $2002 would clear vblank, etc.)
    if (addr < 0x2000)
        return cpuReader(addr);
    else if (addr >= 0x6000 && addr <= 0xFFFF && cartriadge != NULL)
        return cartriadge->mem[cartriadge->mapper(cartriadge, addr)];
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
                     unsigned char (*readCPU)(int),
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
    // printf("%s", cartriadge->mem + 4);
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
void connect_apu_to_bus(unsigned char (*apu_reader)(int), void (*apu_writer)(int, unsigned char))
{
    apuReader = apu_reader;
    apuWriter = apu_writer;
}
void ppu_tick()
{
    tick_ppu();
}

Cartriadge *getCatriadge()
{
    return cartriadge;
}

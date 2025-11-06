#include "bus.h"
#include "registerOffsets.h"

unsigned char readByte(int addr) {return 0;}
void writeByte(int addr, unsigned char value) {}

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
void connectCPUToBus(int (*CPUstatusFlagGetter)(int), void (*CPUstatusFlagSetter)(int, bool), int (*CPUpcGetter)(), void (*CPUpcSetter)(int), void (*CPUstackPush)(unsigned char), unsigned char (*CPUstackPop)()) {
    statusFlagGetter = CPUstatusFlagGetter;
    statusFlagSetter = CPUstatusFlagSetter;
    pcGetter = CPUpcGetter;
    pcSetter = CPUpcSetter;
    stackPush = CPUstackPush;
    stackPop = CPUstackPop;
};

// These call the methods on the CPU, use function pointers to avoid circular deps, maybe messy
int getCPUStatusFlag(int position) {return statusFlagGetter(position);}
void setCPUStatusFlag(int position, bool value) {statusFlagSetter(position, value);}
int getPC() {return pcGetter();}
void setPC(int newPC) {pcSetter(newPC);}
void pushToStack(unsigned char byte) {stackPush(byte);}
unsigned char popFromStack() {return stackPop();}

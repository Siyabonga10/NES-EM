#ifndef BUS_H
#define BUS_H
#include <stdbool.h>
#include "cartriadge.h"

unsigned char readByte(int addr);
void writeByte(int addr, unsigned char value);

unsigned char readBytePPU(int addr);

int getCPUStatusFlag(int position);
void setCPUStatusFlag(int position, bool value);
int getPC();
void setPC(int newPC);

void connectCPUToBus(int (*CPUstatusFlagGetter)(int),
                     void (*CPUstatusFlagSetter)(int, bool),
                     int (*CPUpcGetter)(),
                     void (*CPUpcSetter)(int),
                     void (*CPUstackPush)(unsigned char),
                     unsigned char (*CPUstackPop)(),
                     unsigned char (*readCPU)(int),
                     void (*writeCPU)(int, unsigned char),
                     void (*nmi_trigger)());
void connectCartriadgeToBus(Cartriadge *cart);
void connectController(unsigned char (*controllerReader)(int), void (*controllerWritter)(int, unsigned char));
// return addresses to said registers
int getCPU_Stack();
int getCPU_XRegister();
int getCPU_YRegister();
int getCPU_Accumulator();
int getCPU_StatusRegister();
void pushToStack(unsigned char byte);
void dump6004();
void triggerNMI();
unsigned char popFromStack();

/*=====================================================================================
PPU related functionality
=======================================================================================*/
// TODO: Standardize the names, cant be using different conventions
void connect_ppu_to_bus(void (*ppu_ticker)(), unsigned char (*ppu_reader)(int), void (*ppu_writer)(int, unsigned char));
void connect_apu_to_bus(unsigned char (*apu_reader)(int), void (*apu_writer)(int, unsigned char));
void ppu_tick();
bool is_dma_active();
void update_dma_cycles();

Cartriadge *getCatriadge();
unsigned char fetchFromCPU(int addr);
#endif
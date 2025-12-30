#include "ppu.h"
#include "bus.h"
#include <assert.h>
#include <stdint.h>
#define INTERNAL_REGISTER_SIZE 4
#define EXPOSED_REGISTERS_SIZE 9
#define W_RAM_SIZE 0x2000

static enum InternalReg {
    Internal_V, 
    Internal_T, 
    Internal_X,
    Internal_W
};

static int16_t registers[EXPOSED_REGISTERS_SIZE] = {0};
static unsigned char vram[W_RAM_SIZE] = {0};
static int16_t internal_registers[INTERNAL_REGISTER_SIZE] = {0};
static unsigned char read_buffer = {0};

// These two functions are mainly ever used by the CPU
unsigned char readPPU(int addr)
{
    int register_index = addr - 0x2000;
    assert(register_index < EXPOSED_REGISTERS_SIZE || register_index == 0x2014);
    switch (addr)
    {
    case 0x2002:
        internal_registers[Internal_W] = 0;
        unsigned char status_reg =  (unsigned char)registers[register_index];
        registers[2] &= 0b01111111;
        return status_reg;
    case 0x2004: 
        return (unsigned char)registers[register_index];
        break;
    case 0x2007:
        unsigned char current_read_buff = read_buffer;
        read_buffer = vram[internal_registers[Internal_V]];
        if((registers[0] & 0x2) == 0)
                internal_registers[Internal_V] ++;
            else 
                internal_registers[Internal_V] += 32;
        return current_read_buff;
        break;
    }
}

void writePPU(int addr, unsigned char byte)
{
    int register_index = addr - 0x2000;
    assert(register_index < EXPOSED_REGISTERS_SIZE || register_index == 0x2014);
    switch (addr)
    {
        case 0x2007:
            registers[register_index] = byte;
            vram[internal_registers[Internal_V]] = byte;
            if((registers[0] & 0x2) == 0)
                internal_registers[Internal_V] ++;
            else 
                internal_registers[Internal_V] += 32;

            break;
        case 0x2000:  case 0x2001: case 0x2003: case 0x2004: 
            registers[register_index] = byte;
            break;
        case 0x2005: 
            // TODO: Fix scrolling
            break;
        case 0x2006:
            if(internal_registers[Internal_W] == 0) // Write high byte
            {
                registers[register_index] &= 0x00FF;
                registers[register_index] |= (byte << 8);
                internal_registers[Internal_T] &= 0x00FF;
                internal_registers[Internal_T] |= (byte << 8);
                internal_registers[Internal_W] = 1;
            }
            else
            {
                registers[register_index] &= 0xFF00;
                registers[register_index] |= byte;
                internal_registers[Internal_T] &= 0xFF00;
                internal_registers[Internal_T] |= byte ;
                internal_registers[Internal_V] = internal_registers[Internal_T];
                internal_registers[Internal_W] = 0;
            }
            break;
        case 0x4014:
            registers[8] = byte;
            break;

    }
}

void tick() {

}
void bootPPU()
{
    connect_ppu_to_bus(tick);
}

static void writeTo(int addr, unsigned char value) {

}

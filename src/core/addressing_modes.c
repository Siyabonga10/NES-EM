#include "addressing_modes.h"
#include "bus.h"

// The addressing modes do not offset the PC, this is handled by the CPU

int ABS_A(int PC) { return (read_byte(PC) + ((int)read_byte(PC + 1) << 8)) & 0xFFFF; }
int ABS_INDEX_X(int PC) {
    return (ABS_A(PC) + (int)read_byte(get_cpu_x_register())) & 0xFFFF;
}
int ABS_INDEX_Y(int PC) {
    return (ABS_A(PC) + (int)read_byte(get_cpu_y_register())) & 0xFFFF;
}
int ACC(int PC) {return get_cpu_accumulator();}
int IMM(int PC) {return PC;}
int IMP(int PC) {return -1;}
int STK(int PC) {return (1 << 8) + read_byte(get_cpu_stack());}
int ZP(int PC)  {return read_byte(PC);}
int ZP_INDX_IND(int PC) {
    int zp = read_byte(PC);
    int index = read_byte(get_cpu_x_register());
    int baseAddr = (zp + index) & 0xFF;
    
    int lowByte = read_byte(baseAddr);
    int highByte = read_byte((baseAddr + 1) & 0xFF); 
    
    return lowByte + (highByte << 8);
}
int ZP_INDX_X(int PC) {
    int effectiveAddr = read_byte(PC) + read_byte(get_cpu_x_register());
    return effectiveAddr & 0xFF;
}
int ZP_INDX_Y(int PC){
    int effectiveAddr = read_byte(PC) + read_byte(get_cpu_y_register());
    return effectiveAddr & 0xFF;
}
int ZP_IND(int PC) {
    int indirectAddr = read_byte(PC);
    int lowByte = read_byte(indirectAddr);
    int highByte = read_byte((indirectAddr + 1) & 0xFF);  // Wrap within zero page
    return lowByte + (highByte << 8);
}

int ZP_IND_INDX_Y(int PC) {
    unsigned char zp = read_byte(PC);
    unsigned char y = read_byte(get_cpu_y_register());
    int indirectBaseAddr = read_byte(zp) + ((int)read_byte((zp + 1) & 0xFF) << 8);
    indirectBaseAddr += y;
    return indirectBaseAddr & 0xFFFF;
}

/*
Absolute indexed indirect and absolute indirect is only used with the Jump instr, it modifies the PC, 
this will be handled inside the instruction rather than here
*/
int ABS_IND(int PC) {return PC;}
int ABS_INDX_IND(int PC) {return PC;}
/*
    This is also only
*/
int PCR(int PC) {return PC;}
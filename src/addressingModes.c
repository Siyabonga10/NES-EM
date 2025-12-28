#include "addressingModes.h"
#include "bus.h"

// The addressing modes do not offset the PC, this is handled by the CPU

int ABS_A(int PC) { return (readByte(PC) + ((int)readByte(PC + 1) << 8) & 0xFFFF); }
int ABS_INDEX_X(int PC) {
    //printf("ABS_PC")
    return (ABS_A(PC) + (int)readByte(getCPU_XRegister())) & 0xFFFF;
}
int ABS_INDEX_Y(int PC) {
    return (ABS_A(PC) + (int)readByte(getCPU_YRegister())) & 0xFFFF;
}
int ACC(int PC) {return getCPU_Accumulator();}
int IMM(int PC) {return PC;}
int IMP(int PC) {return -1;}
int STK(int PC) {return (1 << 8) + readByte(getCPU_Stack());}
int ZP(int PC)  {return readByte(PC);}
int ZP_INDX_IND(int PC) {
    int zp = readByte(PC);
    int index = readByte(getCPU_XRegister());
    int baseAddr = (zp + index) & 0xFF;
    
    int lowByte = readByte(baseAddr);
    int highByte = readByte((baseAddr + 1) & 0xFF); 
    
    return lowByte + (highByte << 8);
}
int ZP_INDX_X(int PC) {
    int effectiveAddr = readByte(PC) + readByte(getCPU_XRegister());
    return effectiveAddr & 0xFF;
}
int ZP_INDX_Y(int PC){
    int effectiveAddr = readByte(PC) + readByte(getCPU_YRegister());
    return effectiveAddr & 0xFF;
}
int ZP_IND(int PC) {
    int indirectAddr = readByte(PC);
    int lowByte = readByte(indirectAddr);
    int highByte = readByte((indirectAddr + 1) & 0xFF);  // Wrap within zero page
    return lowByte + (highByte << 8);
}

int ZP_IND_INDX_Y(int PC) {
    unsigned char zp = readByte(PC);
    unsigned char y = readByte(getCPU_YRegister());
    int indirectBaseAddr = readByte(zp) + ((int)readByte((zp + 1) & 0xFF) << 8);
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
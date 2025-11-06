#include "addressingModes.h"
#include "bus.h"

// The addressing modes do not offset the PC, this is handled by the CPU

int ABS_A(int PC) { return readByte(PC) + ((int)readByte(PC + 1) << 8); }
int ABS_INDEX_X(int PC) {return PC + readByte(getCPU_XRegister());}
int ABS_INDEX_Y(int PC) {return PC + readByte(getCPU_YRegister());}
int ACC(int PC) {return getCPU_Accumulator();}
int IMM(int PC) {return PC;}
int IMP(int PC) {return -1;}
int STK(int PC) {return 1 << 8 + readByte(getCPU_Stack());}
int ZP(int PC)  {return readByte(PC);}
int ZP_INDX_IND(int PC) {
    int indirectAddr = readByte(PC) + readByte(getCPU_XRegister());
    indirectAddr &= 0xFF;
    return readByte(indirectAddr) + ((int)readByte(indirectAddr + 1) << 8);
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
    return readByte(indirectAddr) + ((int)readByte(indirectAddr + 1) << 8);
}

int ZP_IND_INDX_Y(int PC) {
    int indirectAddr = readByte(PC) + readByte(getCPU_YRegister());
    indirectAddr &= 0xFF;
    return readByte(indirectAddr) + ((int)readByte(indirectAddr + 1) << 8);
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
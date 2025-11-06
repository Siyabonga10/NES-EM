#include "instructions.h"
#include "bus.h"
#include "addressingModes.h"
#include <stdlib.h>

static ExecutionInfo lastInstruction = (ExecutionInfo){.addressingMode = NULL, .executor = NULL, .instructionSize = 0, .clockCycles = 0};

// Define all instructions
// General format is to take in the address to the operand, compute the result, potentially having side effects, return the result just in case
unsigned char ADC(int operandAddr){
    unsigned char acc = readByte(getCPU_Accumulator());
    unsigned char mem = readByte(operandAddr);
    int tmp = acc + mem + getCPUStatusFlag(0) ? 1 : 0;

    setCPUStatusFlag(0, tmp > 0xFF);
    setCPUStatusFlag(1, tmp == 0);
    setCPUStatusFlag(6, (tmp ^ acc) & (tmp ^ mem) & 0x80);
    setCPUStatusFlag(7, tmp & (1 << 7));
    writeByte(getCPU_Accumulator(), (unsigned char)tmp);
    return tmp;
    
}
unsigned char AND(int operandAddr) {
    unsigned char result = readByte(operandAddr) & readByte(getCPU_Accumulator());
    writeByte(getCPU_Accumulator(), result);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(7, result & (1 << 7));
    return result;
}
unsigned char ASL(int operandAddr) {
    unsigned char operand = readByte(operandAddr);
    setCPUStatusFlag(0, operand & (1 << 7));
    operand <<= 1;
    setCPUStatusFlag(1, operand == 0);
    setCPUStatusFlag(7, operand & (1 << 7));
    writeByte(operandAddr, operand);
    return operand;
}

unsigned char BCC(int operandAddr) {
    if(!getCPUStatusFlag(0))
        setPC(getPC() + 1 + (char)readByte(getPC()));
    return 0;
}
unsigned char BCS(int operandAddr){
    if(getCPUStatusFlag(0))
        setPC(getPC() + 1 + (char)readByte(getPC()));
    return 0;
}
unsigned char BEQ(int operandAddr){
    if(getCPUStatusFlag(1))
        setPC(getPC() + 1 + (char)readByte(getPC()));
    return 0;
}
unsigned char BIT(int operandAddr) {
    unsigned char result = readByte(getCPU_Accumulator()) & readByte(operandAddr);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(6, result & (1 << 6));
    setCPUStatusFlag(7, result & (1 << 7));
    return result;
}
unsigned char BMI(int operandAddr){
    if(getCPUStatusFlag(7))
        setPC(getPC() + 1 + (char)readByte(getPC()));
    return 0;
}
unsigned char BNE(int operandAddr){
    if(!getCPUStatusFlag(1))
        setPC(getPC() + 1 + (char)readByte(getPC()));
    return 0;
}
unsigned char BPL(int operandAddr){
    if(!getCPUStatusFlag(7))
        setPC(getPC() + 1 + (char)readByte(getPC()));
    return 0;
}
unsigned char BVC(int operandAddr){
    if(!getCPUStatusFlag(6))
        setPC(getPC() + 1 + (char)readByte(getPC()));
    return 0;
}
unsigned char BVS(int operandAddr){
    if(getCPUStatusFlag(6))
        setPC(getPC() + 1 + (char)readByte(getPC()));
    return 0;
}

unsigned char CLC(int operandAddr){
    setCPUStatusFlag(0, false);
    return 0;
}
unsigned char CLD(int operandAddr){
    setCPUStatusFlag(3, false);
    return 0;
}
unsigned char CLI(int operandAddr){
    setCPUStatusFlag(2, false); // Delayed by one instrcution, not sure how to implement that for now I will skip
    return 0;
}
unsigned char CLV(int operandAddr){
    setCPUStatusFlag(6, false);
    return 0;
}
unsigned char CMP(int operandAddr){
    unsigned char A = readByte(getCPU_Accumulator());
    unsigned char memory = readByte(operandAddr);
    int result = A - memory;
    setCPUStatusFlag(0, A >= result);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(0, result & (1 << 7));
    return result;
}
unsigned char CPX(int operandAddr){
    unsigned char X = readByte(getCPU_XRegister());
    unsigned char memory = readByte(operandAddr);
    int result = X - memory;
    setCPUStatusFlag(0, X >= result);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(0, result & (1 << 7));
    return result;
}
unsigned char CPY(int operandAddr){
    unsigned char Y = readByte(getCPU_YRegister());
    unsigned char memory = readByte(operandAddr);
    int result = Y - memory;
    setCPUStatusFlag(0, Y >= result);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(0, result & (1 << 7));
    return result;
}

unsigned char DEC(int operandAddr) {
    unsigned char memory = readByte(operandAddr);
    memory -= 1;
    writeByte(operandAddr, memory);
    setCPUStatusFlag(1, memory == 0);
    setCPUStatusFlag(7, memory & (1 << 7));
    return memory;
}
unsigned char DEX(int operandAddr){
    unsigned char memory = readByte(getCPU_XRegister());
    memory -= 1;
    writeByte(getCPU_XRegister(), memory);
    setCPUStatusFlag(1, memory == 0);
    setCPUStatusFlag(7, memory & (1 << 7));
    return memory;
}
unsigned char DEY(int operandAddr){
    unsigned char memory = readByte(getCPU_YRegister());
    memory -= 1;
    writeByte(getCPU_YRegister(), memory);
    setCPUStatusFlag(1, memory == 0);
    setCPUStatusFlag(7, memory & (1 << 7));

    return memory;
}

unsigned char EOR(int operandAddr){
    unsigned char result = readByte(getCPU_Accumulator()) ^ readByte(operandAddr);
    writeByte(getCPU_Accumulator(), result);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(7, result & (1 << 7));
    return result;
}

unsigned char INC(int operandAddr){
    unsigned char result = readByte(operandAddr);
    result += 1;
    writeByte(operandAddr, result);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(7, result & (1 << 7));
    return result;
}
unsigned char INX(int operandAddr){
    unsigned char result = readByte(getCPU_XRegister());
    result += 1;
    writeByte(getCPU_XRegister(), result);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(7, result & (1 << 7));
    return result;
}
unsigned char INY(int operandAddr){
    unsigned char result = readByte(getCPU_YRegister());
    result += 1;
    writeByte(getCPU_YRegister(), result);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(7, result & (1 << 7));
    return result;
}

unsigned char LDA(int operandAddr) {
    unsigned char memory = readByte(operandAddr);
    writeByte(getCPU_Accumulator(), memory);
    setCPUStatusFlag(1, memory == 0);
    setCPUStatusFlag(7, memory < (1 << 7));
    return memory;
}
unsigned char LDX(int operandAddr){
    unsigned char memory = readByte(operandAddr);
    writeByte(getCPU_XRegister(), memory);
    setCPUStatusFlag(1, memory == 0);
    setCPUStatusFlag(7, memory < (1 << 7));
    return memory;
}
unsigned char LDY(int operandAddr){
    unsigned char memory = readByte(operandAddr);
    writeByte(getCPU_YRegister(), memory);
    setCPUStatusFlag(1, memory == 0);
    setCPUStatusFlag(7, memory < (1 << 7));
    return memory;
}
unsigned char LSR(int operandAddr){
    unsigned char operand = readByte(operandAddr);
    setCPUStatusFlag(0, operand & 1);
    operand >>= 1;
    setCPUStatusFlag(1, operand == 0);
    setCPUStatusFlag(7, 0);
    writeByte(operandAddr, operand);
    return operand;
}

unsigned char NOP(int operandAddr)
{
    return 0;
}

unsigned char ORA(int operandAddr)
{
    unsigned char result = readByte(getCPU_Accumulator()) | readByte(operandAddr);
    writeByte(getCPU_Accumulator(), result);
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(7, result & (1 << 7));
    return result;
}

unsigned char PHA(int operandAddr) {
    unsigned char SP = readByte(getCPU_Stack());
    writeByte(0x100 + SP, readByte(getCPU_Accumulator()));
    writeByte(getCPU_Stack(), SP - 1);
    return readByte(getCPU_Accumulator());
}
unsigned char PHP(int operandAddr){
    unsigned char SP = readByte(getCPU_Stack());
    setCPUStatusFlag(4, true);
    writeByte(0x100 + SP, readByte(getCPU_StatusRegister()));
    writeByte(getCPU_Stack(), SP - 1);
    return readByte(getCPU_StatusRegister());
}
unsigned char PLA(int operandAddr){
    unsigned char SP = readByte(getCPU_Stack());
    writeByte(getCPU_Stack(), SP + 1);
    unsigned char newAcc = readByte(0x100 + SP + 1);
    setCPUStatusFlag(1, newAcc == 0);
    setCPUStatusFlag(7, newAcc & (1 << 7));
    writeByte(getCPU_Accumulator(), newAcc);
    return SP;
}
unsigned char PLP(int operandAddr){
    unsigned char SP = readByte(getCPU_Stack());
    writeByte(getCPU_Stack(), SP + 1);
    unsigned char newStatus = readByte(0x100 + SP + 1);
    writeByte(getCPU_StatusRegister(), newStatus);
    return SP;
}


unsigned char ROL(int operandAddr) {
    unsigned char value = readByte(operandAddr);
    unsigned char carryBit = value & (1 << 7) ? 1 : 0;
    value = (value << 1) + carryBit;
    setCPUStatusFlag(0, carryBit);
    setCPUStatusFlag(1, value == 0);
    setCPUStatusFlag(7, value & (1 << 7));
    return value;
}
unsigned char ROR(int operandAddr){
    unsigned char value = readByte(operandAddr);
    unsigned char carryBit = value & 1 ? 1 : 0;
    value = (value >> 1) + (carryBit << 7);
    setCPUStatusFlag(0, carryBit);
    setCPUStatusFlag(1, value == 0);
    setCPUStatusFlag(7, value & (1 << 7));
    return value;
}
unsigned char SBC(int operandAddr) {
    unsigned char memory = readByte(operandAddr);
    unsigned char A = readByte(getCPU_Accumulator());
    unsigned char C = getCPUStatusFlag(0) ? 1 : 0;
    unsigned char result = A - memory - ~C;
    setCPUStatusFlag(0, ~(result < 0x00));
    setCPUStatusFlag(1, result == 0);
    setCPUStatusFlag(6, (result ^ A) & (result ^ ~memory) & 0x80);
    setCPUStatusFlag(7, result >> 7);
    return result;
}
unsigned char SEC(int operandAddr) {
    setCPUStatusFlag(0, true);
    return 0;
}
unsigned char SED(int operandAddr){
    setCPUStatusFlag(3, true);
    return 0;
}
unsigned char SEI(int operandAddr){
    setCPUStatusFlag(2, true);
    return 0;
}
unsigned char STA(int operandAddr) {
    writeByte(operandAddr, readByte(getCPU_Accumulator()));
    return readByte(getCPU_Accumulator());
}
unsigned char STX(int operandAddr){
    writeByte(operandAddr, readByte(getCPU_XRegister()));
    return readByte(getCPU_XRegister());
}
unsigned char STY(int operandAddr){
    writeByte(operandAddr, readByte(getCPU_YRegister()));
    return readByte(getCPU_YRegister());
}

unsigned char TAX(int operandAddr) {
    unsigned char A = readByte(getCPU_Accumulator());
    writeByte(getCPU_XRegister(), A);
    setCPUStatusFlag(1, A == 0);
    setCPUStatusFlag(7, A >> 7);
    return A;
}
unsigned char TAY(int operandAddr){
    unsigned char A = readByte(getCPU_Accumulator());
    writeByte(getCPU_YRegister(), A);
    setCPUStatusFlag(1, A == 0);
    setCPUStatusFlag(7, A >> 7);
    return A;
}
unsigned char TSX(int operandAddr){
    unsigned char SP = readByte(getCPU_Stack());
    writeByte(getCPU_XRegister(), SP);
    setCPUStatusFlag(1,SP == 0);
    setCPUStatusFlag(7,SP >> 7);
    return SP;
}
unsigned char TXA(int operandAddr){
    unsigned char X = readByte(getCPU_XRegister());
    writeByte(getCPU_Accumulator(), X);
    setCPUStatusFlag(1, X == 0);
    setCPUStatusFlag(7, X >> 7);
    return X;
}
unsigned char TXS(int operandAddr){
    unsigned char X = readByte(getCPU_XRegister());
    writeByte(getCPU_Stack(), X);
    setCPUStatusFlag(1, X == 0);
    setCPUStatusFlag(7, X >> 7);
    return X;
}
unsigned char TYA(int operandAddr){
    unsigned char Y = readByte(getCPU_YRegister());
    writeByte(getCPU_Accumulator(), Y);
    setCPUStatusFlag(1, Y == 0);
    setCPUStatusFlag(7, Y >> 7);
    return Y;
}

unsigned char JMP(int operandAddr) {
    int newPC = readByte(getPC()) + readByte(getPC() + 1) << 8;
    if(lastInstruction.addressingMode == ABS_IND) {
        newPC = readByte(newPC) + readByte(newPC + 1) << 8;
    }
    setPC(newPC);
    return 0;
}
unsigned char JSR(int operandAddr) {
    int newPC = readByte(getPC()) + readByte(getPC() + 1) << 8;
    pushToStack(getPC() >> 8);
    pushToStack(getPC() && 0xFF);
    setPC(newPC);
    return 0;
}
unsigned char RTI(int operandAddr) {
    unsigned char status = popFromStack();
    unsigned char pcLow = popFromStack();
    unsigned char pcHigh = popFromStack();
    writeByte(getCPU_StatusRegister(), status);
    setPC(pcLow + pcHigh << 8);
    return 0;
}
unsigned char RTS(int operandAddr) {
    unsigned char pcLow = popFromStack();
    unsigned char pcHigh = popFromStack();
    setPC(pcLow + (pcHigh << 8) + 1);
    return 0;
}

unsigned char BRK(int operandAddr) {
    pushToStack(getPC() >> 8);
    pushToStack(getPC() & 0x80);
    pushToStack(readByte(getCPU_StatusRegister()));
    setPC(0xFFFE);
    return 0;
}


// Shout out claude, I am not writing this table by hand
static ExecutionInfo lookUpTable[16][16] = {
    // Row 0: 0x00 - 0x0F
    {
        {IMP, BRK, 1, 7},      // 0x00 BRK
        {ZP_INDX_IND, ORA, 2, 6},  // 0x01 ORA (zp,x)
        {IMM, NOP, 2, 2},      // 0x02 NOP (reserved)
        {IMP, NOP, 1, 1},      // 0x03 NOP (reserved)
        {IMP, NOP, 1, 1},       // 0x04 TSB zp
        {ZP, ORA, 2, 3},       // 0x05 ORA zp
        {ZP, ASL, 2, 5},       // 0x06 ASL zp
        {ZP, NOP, 2, 5},       // 0x07 RMB0 (using NOP)
        {IMP, PHP, 1, 3},      // 0x08 PHP
        {IMM, ORA, 2, 2},      // 0x09 ORA #
        {ACC, ASL, 1, 2},      // 0x0A ASL A
        {IMP, NOP, 1, 1},      // 0x0B NOP (reserved)
        {IMP, NOP, 1, 1},     // 0x0C TSB a
        {ABS_A, ORA, 3, 4},    // 0x0D ORA a
        {ABS_A, ASL, 3, 6},    // 0x0E ASL a
        {PCR, NOP, 2, 2}       // 0x0F BBR0 (using NOP)
    },
    // Row 1: 0x10 - 0x1F
    {
        {PCR, BPL, 2, 2},      // 0x10 BPL
        {ZP_IND_INDX_Y, ORA, 2, 5},  // 0x11 ORA (zp),y
        {ZP_IND, ORA, 2, 5},   // 0x12 ORA (zp)
        {IMP, NOP, 1, 1},      // 0x13 NOP (reserved)
        {IMP, NOP, 1, 1},       // 0x14 TRB zp
        {ZP_INDX_X, ORA, 2, 4},// 0x15 ORA zp,x
        {ZP_INDX_X, ASL, 2, 6},// 0x16 ASL zp,x
        {ZP, NOP, 2, 5},       // 0x17 RMB1 (using NOP)
        {IMP, CLC, 1, 2},      // 0x18 CLC
        {ABS_INDEX_Y, ORA, 3, 4},  // 0x19 ORA a,y
        {ACC, INC, 1, 2},      // 0x1A INC A
        {IMP, NOP, 1, 1},      // 0x1B NOP (reserved)
        {IMP, NOP, 1, 1},     // 0x1C TRB a
        {ABS_INDEX_X, ORA, 3, 4},  // 0x1D ORA a,x
        {ABS_INDEX_X, ASL, 3, 7},  // 0x1E ASL a,x
        {PCR, NOP, 2, 2}       // 0x1F BBR1 (using NOP)
    },
    // Row 2: 0x20 - 0x2F
    {
        {ABS_A, JSR, 3, 6},    // 0x20 JSR
        {ZP_INDX_IND, AND, 2, 6},  // 0x21 AND (zp,x)
        {IMM, NOP, 2, 2},      // 0x22 NOP (reserved)
        {IMP, NOP, 1, 1},      // 0x23 NOP (reserved)
        {ZP, BIT, 2, 3},       // 0x24 BIT zp
        {ZP, AND, 2, 3},       // 0x25 AND zp
        {ZP, ROL, 2, 5},       // 0x26 ROL zp
        {ZP, NOP, 2, 5},       // 0x27 RMB2 (using NOP)
        {IMP, PLP, 1, 4},      // 0x28 PLP
        {IMM, AND, 2, 2},      // 0x29 AND #
        {ACC, ROL, 1, 2},      // 0x2A ROL A
        {IMP, NOP, 1, 1},      // 0x2B NOP (reserved)
        {ABS_A, BIT, 3, 4},    // 0x2C BIT a
        {ABS_A, AND, 3, 4},    // 0x2D AND a
        {ABS_A, ROL, 3, 6},    // 0x2E ROL a
        {PCR, NOP, 2, 2}       // 0x2F BBR2 (using NOP)
    },
    // Row 3: 0x30 - 0x3F
    {
        {PCR, BMI, 2, 2},      // 0x30 BMI
        {ZP_IND_INDX_Y, AND, 2, 5},  // 0x31 AND (zp),y
        {ZP_IND, AND, 2, 5},   // 0x32 AND (zp)
        {IMP, NOP, 1, 1},      // 0x33 NOP (reserved)
        {ZP_INDX_X, BIT, 2, 4},// 0x34 BIT zp,x
        {ZP_INDX_X, AND, 2, 4},// 0x35 AND zp,x
        {ZP_INDX_X, ROL, 2, 6},// 0x36 ROL zp,x
        {ZP, NOP, 2, 5},       // 0x37 RMB3 (using NOP)
        {IMP, SEC, 1, 2},      // 0x38 SEC
        {ABS_INDEX_Y, AND, 3, 4},  // 0x39 AND a,y
        {ACC, DEC, 1, 2},      // 0x3A DEC A
        {IMP, NOP, 1, 1},      // 0x3B NOP (reserved)
        {ABS_INDEX_X, BIT, 3, 4},  // 0x3C BIT a,x
        {ABS_INDEX_X, AND, 3, 4},  // 0x3D AND a,x
        {ABS_INDEX_X, ROL, 3, 7},  // 0x3E ROL a,x
        {PCR, NOP, 2, 2}       // 0x3F BBR3 (using NOP)
    },
    // Row 4: 0x40 - 0x4F
    {
        {IMP, RTI, 1, 6},      // 0x40 RTI
        {ZP_INDX_IND, EOR, 2, 6},  // 0x41 EOR (zp,x)
        {IMM, NOP, 2, 2},      // 0x42 NOP (reserved)
        {IMP, NOP, 1, 1},      // 0x43 NOP (reserved)
        {ZP, NOP, 2, 3},       // 0x44 NOP (reserved)
        {ZP, EOR, 2, 3},       // 0x45 EOR zp
        {ZP, LSR, 2, 5},       // 0x46 LSR zp
        {ZP, NOP, 2, 5},       // 0x47 RMB4 (using NOP)
        {IMP, PHA, 1, 3},      // 0x48 PHA
        {IMM, EOR, 2, 2},      // 0x49 EOR #
        {ACC, LSR, 1, 2},      // 0x4A LSR A
        {IMP, NOP, 1, 1},      // 0x4B NOP (reserved)
        {ABS_A, JMP, 3, 3},    // 0x4C JMP a
        {ABS_A, EOR, 3, 4},    // 0x4D EOR a
        {ABS_A, LSR, 3, 6},    // 0x4E LSR a
        {PCR, NOP, 2, 2}       // 0x4F BBR4 (using NOP)
    },
    // Row 5: 0x50 - 0x5F
    {
        {PCR, BVC, 2, 2},      // 0x50 BVC
        {ZP_IND_INDX_Y, EOR, 2, 5},  // 0x51 EOR (zp),y
        {ZP_IND, EOR, 2, 5},   // 0x52 EOR (zp)
        {IMP, NOP, 1, 1},      // 0x53 NOP (reserved)
        {ZP_INDX_X, NOP, 2, 4},// 0x54 NOP (reserved)
        {ZP_INDX_X, EOR, 2, 4},// 0x55 EOR zp,x
        {ZP_INDX_X, LSR, 2, 6},// 0x56 LSR zp,x
        {ZP, NOP, 2, 5},       // 0x57 RMB5 (using NOP)
        {IMP, CLI, 1, 2},      // 0x58 CLI
        {ABS_INDEX_Y, EOR, 3, 4},  // 0x59 EOR a,y
        {IMP, NOP, 1, 3},      // 0x5A PHY (using NOP placeholder)
        {IMP, NOP, 1, 1},      // 0x5B NOP (reserved)
        {ABS_A, NOP, 3, 8},    // 0x5C NOP (reserved)
        {ABS_INDEX_X, EOR, 3, 4},  // 0x5D EOR a,x
        {ABS_INDEX_X, LSR, 3, 7},  // 0x5E LSR a,x
        {PCR, NOP, 2, 2}       // 0x5F BBR5 (using NOP)
    },
    // Row 6: 0x60 - 0x6F
    {
        {IMP, RTS, 1, 6},      // 0x60 RTS
        {ZP_INDX_IND, ADC, 2, 6},  // 0x61 ADC (zp,x)
        {IMM, NOP, 2, 2},      // 0x62 NOP (reserved)
        {IMP, NOP, 1, 1},      // 0x63 NOP (reserved)
        {ZP, NOP, 2, 3},       // 0x64 STZ zp (using NOP placeholder)
        {ZP, ADC, 2, 3},       // 0x65 ADC zp
        {ZP, ROR, 2, 5},       // 0x66 ROR zp
        {ZP, NOP, 2, 5},       // 0x67 RMB6 (using NOP)
        {IMP, PLA, 1, 4},      // 0x68 PLA
        {IMM, ADC, 2, 2},      // 0x69 ADC #
        {ACC, ROR, 1, 2},      // 0x6A ROR A
        {IMP, NOP, 1, 1},      // 0x6B NOP (reserved)
        {ABS_IND, JMP, 3, 6},  // 0x6C JMP (a)
        {ABS_A, ADC, 3, 4},    // 0x6D ADC a
        {ABS_A, ROR, 3, 6},    // 0x6E ROR a
        {PCR, NOP, 2, 2}       // 0x6F BBR6 (using NOP)
    },
    // Row 7: 0x70 - 0x7F
    {
        {PCR, BVS, 2, 2},      // 0x70 BVS
        {ZP_IND_INDX_Y, ADC, 2, 5},  // 0x71 ADC (zp),y
        {ZP_IND, ADC, 2, 5},   // 0x72 ADC (zp)
        {IMP, NOP, 1, 1},      // 0x73 NOP (reserved)
        {ZP_INDX_X, NOP, 2, 4},// 0x74 STZ zp,x (using NOP placeholder)
        {ZP_INDX_X, ADC, 2, 4},// 0x75 ADC zp,x
        {ZP_INDX_X, ROR, 2, 6},// 0x76 ROR zp,x
        {ZP, NOP, 2, 5},       // 0x77 RMB7 (using NOP)
        {IMP, SEI, 1, 2},      // 0x78 SEI
        {ABS_INDEX_Y, ADC, 3, 4},  // 0x79 ADC a,y
        {IMP, NOP, 1, 4},      // 0x7A PLY (using NOP placeholder)
        {IMP, NOP, 1, 1},      // 0x7B NOP (reserved)
        {ABS_INDX_IND, JMP, 3, 6},  // 0x7C JMP (a,x)
        {ABS_INDEX_X, ADC, 3, 4},  // 0x7D ADC a,x
        {ABS_INDEX_X, ROR, 3, 7},  // 0x7E ROR a,x
        {PCR, NOP, 2, 2}       // 0x7F BBR7 (using NOP)
    },
    // Row 8: 0x80 - 0x8F
    {
        {PCR, NOP, 2, 3},      // 0x80 BRA (using NOP placeholder)
        {ZP_INDX_IND, STA, 2, 6},  // 0x81 STA (zp,x)
        {IMM, NOP, 2, 2},      // 0x82 NOP (reserved)
        {IMP, NOP, 1, 1},      // 0x83 NOP (reserved)
        {ZP, STY, 2, 3},       // 0x84 STY zp
        {ZP, STA, 2, 3},       // 0x85 STA zp
        {ZP, STX, 2, 3},       // 0x86 STX zp
        {ZP, NOP, 2, 5},       // 0x87 SMB0 (using NOP)
        {IMP, DEY, 1, 2},      // 0x88 DEY
        {IMM, NOP, 2, 2},      // 0x89 BIT # (using NOP placeholder)
        {IMP, TXA, 1, 2},      // 0x8A TXA
        {IMP, NOP, 1, 1},      // 0x8B NOP (reserved)
        {ABS_A, STY, 3, 4},    // 0x8C STY a
        {ABS_A, STA, 3, 4},    // 0x8D STA a
        {ABS_A, STX, 3, 4},    // 0x8E STX a
        {PCR, NOP, 2, 2}       // 0x8F BBS0 (using NOP)
    },
    // Row 9: 0x90 - 0x9F
    {
        {PCR, BCC, 2, 2},      // 0x90 BCC
        {ZP_IND_INDX_Y, STA, 2, 6},  // 0x91 STA (zp),y
        {ZP_IND, STA, 2, 5},   // 0x92 STA (zp)
        {IMP, NOP, 1, 1},      // 0x93 NOP (reserved)
        {ZP_INDX_X, STY, 2, 4},// 0x94 STY zp,x
        {ZP_INDX_X, STA, 2, 4},// 0x95 STA zp,x
        {ZP_INDX_Y, STX, 2, 4},// 0x96 STX zp,y
        {ZP, NOP, 2, 5},       // 0x97 SMB1 (using NOP)
        {IMP, TYA, 1, 2},      // 0x98 TYA
        {ABS_INDEX_Y, STA, 3, 5},  // 0x99 STA a,y
        {IMP, TXS, 1, 2},      // 0x9A TXS
        {IMP, NOP, 1, 1},      // 0x9B NOP (reserved)
        {ABS_A, NOP, 3, 4},    // 0x9C STZ a (using NOP placeholder)
        {ABS_INDEX_X, STA, 3, 5},  // 0x9D STA a,x
        {ABS_INDEX_X, NOP, 3, 5},  // 0x9E STZ a,x (using NOP placeholder)
        {PCR, NOP, 2, 2}       // 0x9F BBS1 (using NOP)
    },
    // Row A: 0xA0 - 0xAF
    {
        {IMM, LDY, 2, 2},      // 0xA0 LDY #
        {ZP_INDX_IND, LDA, 2, 6},  // 0xA1 LDA (zp,x)
        {IMM, LDX, 2, 2},      // 0xA2 LDX #
        {IMP, NOP, 1, 1},      // 0xA3 NOP (reserved)
        {ZP, LDY, 2, 3},       // 0xA4 LDY zp
        {ZP, LDA, 2, 3},       // 0xA5 LDA zp
        {ZP, LDX, 2, 3},       // 0xA6 LDX zp
        {ZP, NOP, 2, 5},       // 0xA7 SMB2 (using NOP)
        {IMP, TAY, 1, 2},      // 0xA8 TAY
        {IMM, LDA, 2, 2},      // 0xA9 LDA #
        {IMP, TAX, 1, 2},      // 0xAA TAX
        {IMP, NOP, 1, 1},      // 0xAB NOP (reserved)
        {ABS_A, LDY, 3, 4},    // 0xAC LDY a
        {ABS_A, LDA, 3, 4},    // 0xAD LDA a
        {ABS_A, LDX, 3, 4},    // 0xAE LDX a
        {PCR, NOP, 2, 2}       // 0xAF BBS2 (using NOP)
    },
    // Row B: 0xB0 - 0xBF
    {
        {PCR, BCS, 2, 2},      // 0xB0 BCS
        {ZP_IND_INDX_Y, LDA, 2, 5},  // 0xB1 LDA (zp),y
        {ZP_IND, LDA, 2, 5},   // 0xB2 LDA (zp)
        {IMP, NOP, 1, 1},      // 0xB3 NOP (reserved)
        {ZP_INDX_X, LDY, 2, 4},// 0xB4 LDY zp,x
        {ZP_INDX_X, LDA, 2, 4},// 0xB5 LDA zp,x
        {ZP_INDX_Y, LDX, 2, 4},// 0xB6 LDX zp,y
        {ZP, NOP, 2, 5},       // 0xB7 SMB3 (using NOP)
        {IMP, CLV, 1, 2},      // 0xB8 CLV
        {ABS_INDEX_Y, LDA, 3, 4},  // 0xB9 LDA a,y
        {IMP, TSX, 1, 2},      // 0xBA TSX
        {IMP, NOP, 1, 1},      // 0xBB NOP (reserved)
        {ABS_INDEX_X, LDY, 3, 4},  // 0xBC LDY a,x
        {ABS_INDEX_X, LDA, 3, 4},  // 0xBD LDA a,x
        {ABS_INDEX_Y, LDX, 3, 4},  // 0xBE LDX a,y
        {PCR, NOP, 2, 2}       // 0xBF BBS3 (using NOP)
    },
    // Row C: 0xC0 - 0xCF
    {
        {IMM, CPY, 2, 2},      // 0xC0 CPY #
        {ZP_INDX_IND, CMP, 2, 6},  // 0xC1 CMP (zp,x)
        {IMM, NOP, 2, 2},      // 0xC2 NOP (reserved)
        {IMP, NOP, 1, 1},      // 0xC3 NOP (reserved)
        {ZP, CPY, 2, 3},       // 0xC4 CPY zp
        {ZP, CMP, 2, 3},       // 0xC5 CMP zp
        {ZP, DEC, 2, 5},       // 0xC6 DEC zp
        {ZP, NOP, 2, 5},       // 0xC7 SMB4 (using NOP)
        {IMP, INY, 1, 2},      // 0xC8 INY
        {IMM, CMP, 2, 2},      // 0xC9 CMP #
        {IMP, DEX, 1, 2},      // 0xCA DEX
        {IMP, NOP, 1, 3},      // 0xCB WAI (using NOP placeholder)
        {ABS_A, CPY, 3, 4},    // 0xCC CPY a
        {ABS_A, CMP, 3, 4},    // 0xCD CMP a
        {ABS_A, DEC, 3, 6},    // 0xCE DEC a
        {PCR, NOP, 2, 2}       // 0xCF BBS4 (using NOP)
    },
    // Row D: 0xD0 - 0xDF
    {
        {PCR, BNE, 2, 2},      // 0xD0 BNE
        {ZP_IND_INDX_Y, CMP, 2, 5},  // 0xD1 CMP (zp),y
        {ZP_IND, CMP, 2, 5},   // 0xD2 CMP (zp)
        {IMP, NOP, 1, 1},      // 0xD3 NOP (reserved)
        {ZP_INDX_X, NOP, 2, 4},// 0xD4 NOP (reserved)
        {ZP_INDX_X, CMP, 2, 4},// 0xD5 CMP zp,x
        {ZP_INDX_X, DEC, 2, 6},// 0xD6 DEC zp,x
        {ZP, NOP, 2, 5},       // 0xD7 SMB5 (using NOP)
        {IMP, CLD, 1, 2},      // 0xD8 CLD
        {ABS_INDEX_Y, CMP, 3, 4},  // 0xD9 CMP a,y
        {IMP, NOP, 1, 3},      // 0xDA PHX (using NOP placeholder)
        {IMP, NOP, 1, 3},      // 0xDB STP (using NOP placeholder)
        {ABS_A, NOP, 3, 4},    // 0xDC NOP (reserved)
        {ABS_INDEX_X, CMP, 3, 4},  // 0xDD CMP a,x
        {ABS_INDEX_X, DEC, 3, 7},  // 0xDE DEC a,x
        {PCR, NOP, 2, 2}       // 0xDF BBS5 (using NOP)
    },
    // Row E: 0xE0 - 0xEF
    {
        {IMM, CPX, 2, 2},      // 0xE0 CPX #
        {ZP_INDX_IND, SBC, 2, 6},  // 0xE1 SBC (zp,x)
        {IMM, NOP, 2, 2},      // 0xE2 NOP (reserved)
        {IMP, NOP, 1, 1},      // 0xE3 NOP (reserved)
        {ZP, CPX, 2, 3},       // 0xE4 CPX zp
        {ZP, SBC, 2, 3},       // 0xE5 SBC zp
        {ZP, INC, 2, 5},       // 0xE6 INC zp
        {ZP, NOP, 2, 5},       // 0xE7 SMB6 (using NOP)
        {IMP, INX, 1, 2},      // 0xE8 INX
        {IMM, SBC, 2, 2},      // 0xE9 SBC #
        {IMP, NOP, 1, 2},      // 0xEA NOP
        {IMP, NOP, 1, 1},      // 0xEB NOP (reserved)
        {ABS_A, CPX, 3, 4},    // 0xEC CPX a
        {ABS_A, SBC, 3, 4},    // 0xED SBC a
        {ABS_A, INC, 3, 6},    // 0xEE INC a
        {PCR, NOP, 2, 2}       // 0xEF BBS6 (using NOP)
    },
    // Row F: 0xF0 - 0xFF
    {
        {PCR, BEQ, 2, 2},      // 0xF0 BEQ
        {ZP_IND_INDX_Y, SBC, 2, 5},  // 0xF1 SBC (zp),y
        {ZP_IND, SBC, 2, 5},   // 0xF2 SBC (zp)
        {IMP, NOP, 1, 1},      // 0xF3 NOP (reserved)
        {ZP_INDX_X, NOP, 2, 4},// 0xF4 NOP (reserved)
        {ZP_INDX_X, SBC, 2, 4},// 0xF5 SBC zp,x
        {ZP_INDX_X, INC, 2, 6},// 0xF6 INC zp,x
        {ZP, NOP, 2, 5},       // 0xF7 SMB7 (using NOP)
        {IMP, SED, 1, 2},      // 0xF8 SED
        {ABS_INDEX_Y, SBC, 3, 4},  // 0xF9 SBC a,y
        {IMP, NOP, 1, 4},      // 0xFA PLX (using NOP placeholder)
        {IMP, NOP, 1, 1},      // 0xFB NOP (reserved)
        {ABS_A, NOP, 3, 4},    // 0xFC NOP (reserved)
        {ABS_INDEX_X, SBC, 3, 4},  // 0xFD SBC a,x
        {ABS_INDEX_X, INC, 3, 7},  // 0xFE INC a,x
        {PCR, NOP, 2, 2}       // 0xFF BBS7 (using NOP)
    }
};

ExecutionInfo getExecutionInfo(unsigned char opCode) {
    unsigned char lower = opCode & 0b00001111;
    unsigned char upper = opCode >> 4;
    lastInstruction = lookUpTable[upper][lower];
    return lastInstruction;
}
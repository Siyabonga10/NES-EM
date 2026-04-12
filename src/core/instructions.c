#include "instructions.h"
#include "bus.h"
#include "addressingModes.h"
#include "statusFlag.h"
#include <stdlib.h>
#include <stdio.h>

static ExecutionInfo lastInstruction = (ExecutionInfo){.addressingMode = NULL, .executor = NULL, .instructionSize = 0, .clockCycles = 0};

// Define all instructions
// General format is to take in the address to the operand, compute the result, potentially having side effects, return the result just in case
unsigned char ADC(int operandAddr, int *additionalClockCycles)
{
    unsigned char acc = readByte(getCPU_Accumulator());
    unsigned char mem = readByte(operandAddr);
    int tmp = acc + mem + (getCPUStatusFlag(CARRY) ? 1 : 0);

    setCPUStatusFlag(CARRY, tmp > 0xFF);
    setCPUStatusFlag(ZERO, (unsigned char)tmp == 0);
    setCPUStatusFlag(CPU_OVERFLOW, (tmp ^ acc) & (tmp ^ mem) & 0x80);
    setCPUStatusFlag(NEGATIVE, tmp & (1 << NEGATIVE));
    writeByte(getCPU_Accumulator(), (unsigned char)tmp);
    *additionalClockCycles = 0;
    return tmp;
}

unsigned char AND(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char result = readByte(operandAddr) & readByte(getCPU_Accumulator());
    writeByte(getCPU_Accumulator(), result);
    setCPUStatusFlag(ZERO, result == 0);
    setCPUStatusFlag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char ASL(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char operand = readByte(operandAddr);
    setCPUStatusFlag(CARRY, operand & (1 << 7));
    operand <<= 1;
    setCPUStatusFlag(ZERO, operand == 0);
    setCPUStatusFlag(NEGATIVE, operand & (1 << NEGATIVE));
    writeByte(operandAddr, operand);
    return operand;
}

unsigned char BCC(int operandAddr, int *additionalClockCycles)
{
    if (!getCPUStatusFlag(CARRY))
    {
        setPC(getPC() + (char)readByte(getPC()));
        *additionalClockCycles = 1;
    }
    else
    {
        *additionalClockCycles = 0;
    }
    return 0;
}
unsigned char BCS(int operandAddr, int *additionalClockCycles)
{
    if (getCPUStatusFlag(CARRY))
    {
        setPC(getPC() + (char)readByte(getPC()));
        *additionalClockCycles = 1;
    }
    else
    {
        *additionalClockCycles = 0;
    }
    return 0;
}

unsigned char BEQ(int operandAddr, int *additionalClockCycles)
{
    if (getCPUStatusFlag(ZERO))
    {
        setPC(getPC() + (char)readByte(getPC()));
        *additionalClockCycles = 1;
    }
    else
    {
        *additionalClockCycles = 0;
    }
    return 0;
}
unsigned char BIT(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char memory = readByte(operandAddr);
    unsigned char result = readByte(getCPU_Accumulator()) & memory;
    setCPUStatusFlag(ZERO, result == 0);
    setCPUStatusFlag(CPU_OVERFLOW, memory & (1 << CPU_OVERFLOW));
    setCPUStatusFlag(NEGATIVE, memory & (1 << NEGATIVE));
    return result;
}
unsigned char BMI(int operandAddr, int *additionalClockCycles)
{
    if (getCPUStatusFlag(NEGATIVE))
    {
        setPC(getPC() + (char)readByte(getPC()));
        *additionalClockCycles = 1;
    }
    else
    {
        *additionalClockCycles = 0;
    }
    return 0;
}
unsigned char BNE(int operandAddr, int *additionalClockCycles)
{
    if (!getCPUStatusFlag(ZERO))
    {
        setPC(getPC() + (char)readByte(getPC()));
        *additionalClockCycles = 1;
    }
    else
    {
        *additionalClockCycles = 0;
    }
    return 0;
}
unsigned char BPL(int operandAddr, int *additionalClockCycles)
{
    if (!getCPUStatusFlag(NEGATIVE))
    {
        setPC(getPC() + (char)readByte(getPC()));
        *additionalClockCycles = 1;
    }
    else
    {
        *additionalClockCycles = 0;
    }
    return 0;
}
unsigned char BVC(int operandAddr, int *additionalClockCycles)
{
    if (!getCPUStatusFlag(CPU_OVERFLOW))
    {
        setPC(getPC() + (char)readByte(getPC()));
        *additionalClockCycles = 1;
    }
    else
    {
        *additionalClockCycles = 0;
    }
    return 0;
}
unsigned char BVS(int operandAddr, int *additionalClockCycles)
{
    if (getCPUStatusFlag(CPU_OVERFLOW))
    {
        setPC(getPC() + (char)readByte(getPC()));
        *additionalClockCycles = 1;
    }
    else
    {
        *additionalClockCycles = 0;
    }
    return 0;
}

unsigned char CLC(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    setCPUStatusFlag(CARRY, false);
    return 0;
}
unsigned char CLD(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    setCPUStatusFlag(DECIMAL, false);
    return 0;
}
unsigned char CLI(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    setCPUStatusFlag(INTERRUPT, false); // Delayed by one instrcution, not sure how to implement that for now I will skip
    return 0;
}
unsigned char CLV(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    setCPUStatusFlag(CPU_OVERFLOW, false);
    return 0;
}
unsigned char CMP(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char A = readByte(getCPU_Accumulator());
    unsigned char memory = readByte(operandAddr);
    int result = A - memory;
    setCPUStatusFlag(CARRY, A >= memory);
    setCPUStatusFlag(ZERO, memory == A);
    setCPUStatusFlag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char CPX(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char X = readByte(getCPU_XRegister());
    unsigned char memory = readByte(operandAddr);
    int result = X - memory;
    setCPUStatusFlag(CARRY, X >= memory);
    setCPUStatusFlag(ZERO, X == memory);
    setCPUStatusFlag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char CPY(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char Y = readByte(getCPU_YRegister());
    unsigned char memory = readByte(operandAddr);
    int result = Y - memory;
    setCPUStatusFlag(CARRY, Y >= memory);
    setCPUStatusFlag(ZERO, Y == memory);
    setCPUStatusFlag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}

unsigned char DEC(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char memory = readByte(operandAddr);
    memory -= 1;
    writeByte(operandAddr, memory);
    setCPUStatusFlag(ZERO, memory == 0);
    setCPUStatusFlag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char DEX(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char memory = readByte(getCPU_XRegister());
    memory -= 1;
    writeByte(getCPU_XRegister(), memory);
    setCPUStatusFlag(ZERO, memory == 0);
    setCPUStatusFlag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char DEY(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char memory = readByte(getCPU_YRegister());
    memory -= 1;
    writeByte(getCPU_YRegister(), memory);
    setCPUStatusFlag(ZERO, memory == 0);
    setCPUStatusFlag(NEGATIVE, memory & (1 << NEGATIVE));

    return memory;
}

unsigned char EOR(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char result = readByte(getCPU_Accumulator()) ^ readByte(operandAddr);
    writeByte(getCPU_Accumulator(), result);
    setCPUStatusFlag(ZERO, result == 0);
    setCPUStatusFlag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}

unsigned char INC(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char result = readByte(operandAddr);
    result += 1;
    writeByte(operandAddr, result);
    setCPUStatusFlag(ZERO, result == 0);
    setCPUStatusFlag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char INX(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char result = readByte(getCPU_XRegister());
    result += 1;
    writeByte(getCPU_XRegister(), result);
    setCPUStatusFlag(ZERO, result == 0);
    setCPUStatusFlag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char INY(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char result = readByte(getCPU_YRegister());
    result += 1;
    writeByte(getCPU_YRegister(), result);
    setCPUStatusFlag(ZERO, result == 0);
    setCPUStatusFlag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}

unsigned char LDA(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char memory = readByte(operandAddr);
    writeByte(getCPU_Accumulator(), memory);
    setCPUStatusFlag(ZERO, memory == 0);
    setCPUStatusFlag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char LDX(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char memory = readByte(operandAddr);
    writeByte(getCPU_XRegister(), memory);
    setCPUStatusFlag(ZERO, memory == 0);
    setCPUStatusFlag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char LDY(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char memory = readByte(operandAddr);
    writeByte(getCPU_YRegister(), memory);
    setCPUStatusFlag(ZERO, memory == 0);
    setCPUStatusFlag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char LSR(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char operand = readByte(operandAddr);
    setCPUStatusFlag(CARRY, operand & 1);
    operand >>= 1;
    setCPUStatusFlag(ZERO, operand == 0);
    setCPUStatusFlag(NEGATIVE, 0);
    writeByte(operandAddr, operand);
    return operand;
}

unsigned char NOP(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    return 0;
}

unsigned char ORA(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char result = readByte(getCPU_Accumulator()) | readByte(operandAddr);
    writeByte(getCPU_Accumulator(), result);
    setCPUStatusFlag(ZERO, result == 0);
    setCPUStatusFlag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}

unsigned char PHA(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char A = readByte(getCPU_Accumulator());
    pushToStack(A);
    return A;
}
unsigned char PHP(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char SR = readByte(getCPU_StatusRegister());
    // Set bits 4 and 5 before pushing
    SR |= 0x30; // Set both bit 5 (0x20) and bit 4 (0x10)
    pushToStack(SR);
    return SR;
}
unsigned char PLA(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char A = popFromStack();
    writeByte(getCPU_Accumulator(), A);
    setCPUStatusFlag(ZERO, A == 0);
    setCPUStatusFlag(NEGATIVE, A & (1 << NEGATIVE));
    return A;
}
unsigned char PLP(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char SR = popFromStack();
    // Bit 5 is always 1, bit 4 is ignored (not a real flag)
    SR |= 0x20;  // Ensure bit 5 is set
    SR &= ~0x10; // Clear bit 4 (it's not a real flag)
    writeByte(getCPU_StatusRegister(), SR);
    return SR;
}

unsigned char ROL(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char value = readByte(operandAddr);
    unsigned char initialValue = value;
    unsigned char carryBit = getCPUStatusFlag(CARRY) ? 1 : 0;
    value = (value << 1) + carryBit;
    writeByte(operandAddr, value);
    setCPUStatusFlag(CARRY, initialValue & (1 << 7));
    setCPUStatusFlag(ZERO, value == 0);
    setCPUStatusFlag(NEGATIVE, value & (1 << NEGATIVE));
    return value;
}
unsigned char ROR(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char value = readByte(operandAddr);
    unsigned char initialValue = value;
    unsigned char carryBit = getCPUStatusFlag(CARRY) ? 1 : 0;
    value = (value >> 1) + (carryBit << 7);
    writeByte(operandAddr, value);
    setCPUStatusFlag(CARRY, initialValue & 1);
    setCPUStatusFlag(ZERO, value == 0);
    setCPUStatusFlag(NEGATIVE, value & (1 << NEGATIVE));
    return value;
}
unsigned char SBC(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char memory = readByte(operandAddr);
    unsigned char A = readByte(getCPU_Accumulator());
    unsigned char C = getCPUStatusFlag(CARRY) ? 1 : 0;

    int tmp = A - memory - (1 - C);
    unsigned char result = (unsigned char)tmp;

    writeByte(getCPU_Accumulator(), result);

    setCPUStatusFlag(CARRY, tmp >= 0);
    setCPUStatusFlag(ZERO, result == 0);
    setCPUStatusFlag(CPU_OVERFLOW, (result ^ A) & (result ^ ~memory) & 0x80);
    setCPUStatusFlag(NEGATIVE, result >> NEGATIVE);
    return result;
}
unsigned char SEC(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    setCPUStatusFlag(CARRY, true);
    return 0;
}
unsigned char SED(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    setCPUStatusFlag(DECIMAL, true);
    return 0;
}
unsigned char SEI(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    setCPUStatusFlag(INTERRUPT, true);
    return 0;
}
unsigned char STA(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    writeByte(operandAddr, readByte(getCPU_Accumulator()));
    return readByte(getCPU_Accumulator());
}
unsigned char STX(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    writeByte(operandAddr, readByte(getCPU_XRegister()));
    return readByte(getCPU_XRegister());
}
unsigned char STY(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    writeByte(operandAddr, readByte(getCPU_YRegister()));
    return readByte(getCPU_YRegister());
}

unsigned char TAX(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char A = readByte(getCPU_Accumulator());
    writeByte(getCPU_XRegister(), A);
    setCPUStatusFlag(ZERO, A == 0);
    setCPUStatusFlag(NEGATIVE, A >> NEGATIVE);
    return A;
}
unsigned char TAY(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char A = readByte(getCPU_Accumulator());
    writeByte(getCPU_YRegister(), A);
    setCPUStatusFlag(ZERO, A == 0);
    setCPUStatusFlag(NEGATIVE, A >> NEGATIVE);
    return A;
}
unsigned char TSX(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char SP = readByte(getCPU_Stack());
    writeByte(getCPU_XRegister(), SP);
    setCPUStatusFlag(ZERO, SP == 0);
    setCPUStatusFlag(NEGATIVE, SP >> NEGATIVE);
    return SP;
}
unsigned char TXA(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char X = readByte(getCPU_XRegister());
    writeByte(getCPU_Accumulator(), X);
    setCPUStatusFlag(ZERO, X == 0);
    setCPUStatusFlag(NEGATIVE, X >> NEGATIVE);
    return X;
}
unsigned char TXS(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char X = readByte(getCPU_XRegister());
    writeByte(getCPU_Stack(), X);
    return X;
}
unsigned char TYA(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char Y = readByte(getCPU_YRegister());
    writeByte(getCPU_Accumulator(), Y);
    setCPUStatusFlag(ZERO, Y == 0);
    setCPUStatusFlag(NEGATIVE, Y >> NEGATIVE);
    return Y;
}

static bool pending_nmi = false;
static bool nmi_delayed = false;
void NMI()
{
    pending_nmi = true;
    nmi_delayed = false;
}

void triggerDelayedNMI()
{
    pending_nmi = true;
    nmi_delayed = true;
}

void executeNMI()
{
    int pc = getPC();
    pushToStack(pc >> 8);
    pushToStack(pc & 0xFF);
    unsigned char p_copy = readByte(getCPU_StatusRegister());
    unsigned char mask = 1;
    mask <<= 4;
    mask = ~mask;
    pushToStack(p_copy & mask);
    int low = readByte(0xFFFA);
    int high = ((int)readByte(0xFFFB) << 8);
    setPC(low + high);
    setCPUStatusFlag(INTERRUPT, true);
    pending_nmi = false;
}

bool pendingNMI()
{
    return pending_nmi && !nmi_delayed;
}

void cpu_instruction_completed()
{
    if (nmi_delayed)
    {
        fprintf(stderr, "[CPU] Instruction completed, delayed NMI cleared\n");
    }
    nmi_delayed = false;
}

unsigned char JMP(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    int newPC = readByte(getPC()) + ((int)readByte(getPC() + 1) << 8);
    if (lastInstruction.addressingMode == ABS_IND)
    {
        int secondAddr = newPC;
        if ((secondAddr & 0xFF) == 0xFF)
            secondAddr -= 0xFF;
        else
            secondAddr += 1;
        newPC = readByte(newPC) + ((int)readByte(secondAddr) << 8);
    }
    setPC(newPC);
    return 0;
}
unsigned char JSR(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    int newPC = readByte(getPC()) + ((int)readByte(getPC() + 1) << 8);
    int pc = getPC() + 1;
    pushToStack(pc >> 8);
    pushToStack(pc & 0xFF);
    setPC(newPC);
    return 0;
}
unsigned char RTI(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char status = popFromStack();
    unsigned char pcLow = popFromStack();
    unsigned char pcHigh = popFromStack();
    writeByte(getCPU_StatusRegister(), status);
    setPC(pcLow + ((int)pcHigh << 8));
    return 0;
}
unsigned char RTS(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    unsigned char pcLow = popFromStack();
    unsigned char pcHigh = popFromStack();
    setPC(pcLow + ((int)pcHigh << 8) + 1);
    return 0;
}

unsigned char BRK(int operandAddr, int *additionalClockCycles)
{
    *additionalClockCycles = 0;
    int pc = getPC() + 1;
    pushToStack(pc >> 8);
    pushToStack(pc & 0xFF);
    setCPUStatusFlag(4, true);
    setCPUStatusFlag(5, true);
    pushToStack(readByte(getCPU_StatusRegister()));
    setPC(readByte(0xFFFE) + ((int)readByte(0xFFFE + 1) << 8));
    return 0;
}

// Shout out claude, I am not writing this table by hand
static ExecutionInfo lookUpTable[16][16] = {
    // Row 0: 0x00 - 0x0F
    {
        {IMP, BRK, 1, 7},         // 0x00 BRK
        {ZP_INDX_IND, ORA, 2, 6}, // 0x01 ORA (zp,x)
        {IMP, NOP, 2, 2},         // 0x02 *KIL (treat as 1-byte NOP that halts - changed cycles)
        {IMP, NOP, 2, 8},         // 0x03 *SLO (zp,x) - unofficial
        {IMP, NOP, 2, 3},         // 0x04 *NOP zp - unofficial
        {ZP, ORA, 2, 3},          // 0x05 ORA zp
        {ZP, ASL, 2, 5},          // 0x06 ASL zp
        {IMP, NOP, 2, 5},         // 0x07 *SLO zp - unofficial
        {IMP, PHP, 1, 3},         // 0x08 PHP
        {IMM, ORA, 2, 2},         // 0x09 ORA #
        {ACC, ASL, 1, 2},         // 0x0A ASL A
        {IMP, NOP, 2, 2},         // 0x0B *ANC # - unofficial
        {IMP, NOP, 3, 4},         // 0x0C *NOP a - unofficial
        {ABS_A, ORA, 3, 4},       // 0x0D ORA a
        {ABS_A, ASL, 3, 6},       // 0x0E ASL a
        {IMP, NOP, 3, 6},         // 0x0F *SLO a - unofficial
    },
    // Row 1: 0x10 - 0x1F
    {
        {PCR, BPL, 2, 2},           // 0x10 BPL
        {ZP_IND_INDX_Y, ORA, 2, 5}, // 0x11 ORA (zp),y
        {ZP_IND, ORA, 2, 5},        // 0x12 ORA (zp)
        {IMP, NOP, 2, 8},           // 0x13 *SLO (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0x14 *NOP zp,x - unofficial
        {ZP_INDX_X, ORA, 2, 4},     // 0x15 ORA zp,x
        {ZP_INDX_X, ASL, 2, 6},     // 0x16 ASL zp,x
        {IMP, NOP, 2, 6},           // 0x17 *SLO zp,x - unofficial
        {IMP, CLC, 1, 2},           // 0x18 CLC
        {ABS_INDEX_Y, ORA, 3, 4},   // 0x19 ORA a,y
        {ACC, INC, 1, 2},           // 0x1A INC A
        {IMP, NOP, 3, 7},           // 0x1B *SLO a,y - unofficial
        {IMP, NOP, 3, 4},           // 0x1C *NOP a,x - unofficial
        {ABS_INDEX_X, ORA, 3, 4},   // 0x1D ORA a,x
        {ABS_INDEX_X, ASL, 3, 7},   // 0x1E ASL a,x
        {IMP, NOP, 3, 7},           // 0x1F *SLO a,x - unofficial
    },
    // Row 2: 0x20 - 0x2F
    {
        {ABS_A, JSR, 1, 6},       // 0x20 JSR
        {ZP_INDX_IND, AND, 2, 6}, // 0x21 AND (zp,x)
        {IMP, NOP, 1, 2},         // 0x22 *KIL - unofficial
        {IMP, NOP, 2, 8},         // 0x23 *RLA (zp,x) - unofficial
        {ZP, BIT, 2, 3},          // 0x24 BIT zp
        {ZP, AND, 2, 3},          // 0x25 AND zp
        {ZP, ROL, 2, 5},          // 0x26 ROL zp
        {IMP, NOP, 2, 5},         // 0x27 *RLA zp - unofficial
        {IMP, PLP, 1, 4},         // 0x28 PLP
        {IMM, AND, 2, 2},         // 0x29 AND #
        {ACC, ROL, 1, 2},         // 0x2A ROL A
        {IMP, NOP, 2, 2},         // 0x2B *ANC # - unofficial
        {ABS_A, BIT, 3, 4},       // 0x2C BIT a
        {ABS_A, AND, 3, 4},       // 0x2D AND a
        {ABS_A, ROL, 3, 6},       // 0x2E ROL a
        {IMP, NOP, 3, 6},         // 0x2F *RLA a - unofficial
    },
    // Row 3: 0x30 - 0x3F
    {
        {PCR, BMI, 2, 2},           // 0x30 BMI
        {ZP_IND_INDX_Y, AND, 2, 5}, // 0x31 AND (zp),y
        {ZP_IND, AND, 2, 5},        // 0x32 AND (zp)
        {IMP, NOP, 2, 8},           // 0x33 *RLA (zp),y - unofficial
        {ZP_INDX_X, BIT, 2, 4},     // 0x34 BIT zp,x
        {ZP_INDX_X, AND, 2, 4},     // 0x35 AND zp,x
        {ZP_INDX_X, ROL, 2, 6},     // 0x36 ROL zp,x
        {IMP, NOP, 2, 6},           // 0x37 *RLA zp,x - unofficial
        {IMP, SEC, 1, 2},           // 0x38 SEC
        {ABS_INDEX_Y, AND, 3, 4},   // 0x39 AND a,y
        {ACC, DEC, 1, 2},           // 0x3A DEC A
        {IMP, NOP, 3, 7},           // 0x3B *RLA a,y - unofficial
        {ABS_INDEX_X, BIT, 3, 4},   // 0x3C BIT a,x
        {ABS_INDEX_X, AND, 3, 4},   // 0x3D AND a,x
        {ABS_INDEX_X, ROL, 3, 7},   // 0x3E ROL a,x
        {IMP, NOP, 3, 7},           // 0x3F *RLA a,x - unofficial
    },
    // Row 4: 0x40 - 0x4F
    {
        {IMP, RTI, 1, 6},         // 0x40 RTI
        {ZP_INDX_IND, EOR, 2, 6}, // 0x41 EOR (zp,x)
        {IMP, NOP, 1, 2},         // 0x42 *KIL - unofficial
        {IMP, NOP, 2, 8},         // 0x43 *SRE (zp,x) - unofficial
        {IMP, NOP, 2, 3},         // 0x44 *NOP zp - unofficial
        {ZP, EOR, 2, 3},          // 0x45 EOR zp
        {ZP, LSR, 2, 5},          // 0x46 LSR zp
        {IMP, NOP, 2, 5},         // 0x47 *SRE zp - unofficial
        {IMP, PHA, 1, 3},         // 0x48 PHA
        {IMM, EOR, 2, 2},         // 0x49 EOR #
        {ACC, LSR, 1, 2},         // 0x4A LSR A
        {IMP, NOP, 2, 2},         // 0x4B *ALR # - unofficial
        {ABS_A, JMP, 1, 3},       // 0x4C JMP a
        {ABS_A, EOR, 3, 4},       // 0x4D EOR a
        {ABS_A, LSR, 3, 6},       // 0x4E LSR a
        {IMP, NOP, 3, 6},         // 0x4F *SRE a - unofficial
    },
    // Row 5: 0x50 - 0x5F
    {
        {PCR, BVC, 2, 2},           // 0x50 BVC
        {ZP_IND_INDX_Y, EOR, 2, 5}, // 0x51 EOR (zp),y
        {ZP_IND, EOR, 2, 5},        // 0x52 EOR (zp)
        {IMP, NOP, 2, 8},           // 0x53 *SRE (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0x54 *NOP zp,x - unofficial
        {ZP_INDX_X, EOR, 2, 4},     // 0x55 EOR zp,x
        {ZP_INDX_X, LSR, 2, 6},     // 0x56 LSR zp,x
        {IMP, NOP, 2, 6},           // 0x57 *SRE zp,x - unofficial
        {IMP, CLI, 1, 2},           // 0x58 CLI
        {ABS_INDEX_Y, EOR, 3, 4},   // 0x59 EOR a,y
        {IMP, NOP, 1, 2},           // 0x5A *NOP - unofficial
        {IMP, NOP, 3, 7},           // 0x5B *SRE a,y - unofficial
        {IMP, NOP, 3, 4},           // 0x5C *NOP a,x - unofficial
        {ABS_INDEX_X, EOR, 3, 4},   // 0x5D EOR a,x
        {ABS_INDEX_X, LSR, 3, 7},   // 0x5E LSR a,x
        {IMP, NOP, 3, 7},           // 0x5F *SRE a,x - unofficial
    },
    // Row 6: 0x60 - 0x6F
    {
        {IMP, RTS, 1, 6},         // 0x60 RTS
        {ZP_INDX_IND, ADC, 2, 6}, // 0x61 ADC (zp,x)
        {IMP, NOP, 1, 2},         // 0x62 *KIL - unofficial
        {IMP, NOP, 2, 8},         // 0x63 *RRA (zp,x) - unofficial
        {IMP, NOP, 2, 3},         // 0x64 *NOP zp - unofficial
        {ZP, ADC, 2, 3},          // 0x65 ADC zp
        {ZP, ROR, 2, 5},          // 0x66 ROR zp
        {IMP, NOP, 2, 5},         // 0x67 *RRA zp - unofficial
        {IMP, PLA, 1, 4},         // 0x68 PLA
        {IMM, ADC, 2, 2},         // 0x69 ADC #
        {ACC, ROR, 1, 2},         // 0x6A ROR A
        {IMP, NOP, 2, 2},         // 0x6B *ARR # - unofficial
        {ABS_IND, JMP, 1, 6},     // 0x6C JMP (a)
        {ABS_A, ADC, 3, 4},       // 0x6D ADC a
        {ABS_A, ROR, 3, 6},       // 0x6E ROR a
        {IMP, NOP, 3, 6},         // 0x6F *RRA a - unofficial
    },
    // Row 7: 0x70 - 0x7F
    {
        {PCR, BVS, 2, 2},           // 0x70 BVS
        {ZP_IND_INDX_Y, ADC, 2, 5}, // 0x71 ADC (zp),y
        {ZP_IND, ADC, 2, 5},        // 0x72 ADC (zp)
        {IMP, NOP, 2, 8},           // 0x73 *RRA (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0x74 *NOP zp,x - unofficial
        {ZP_INDX_X, ADC, 2, 4},     // 0x75 ADC zp,x
        {ZP_INDX_X, ROR, 2, 6},     // 0x76 ROR zp,x
        {IMP, NOP, 2, 6},           // 0x77 *RRA zp,x - unofficial
        {IMP, SEI, 1, 2},           // 0x78 SEI
        {ABS_INDEX_Y, ADC, 3, 4},   // 0x79 ADC a,y
        {IMP, NOP, 1, 2},           // 0x7A *NOP - unofficial
        {IMP, NOP, 3, 7},           // 0x7B *RRA a,y - unofficial
        {IMP, NOP, 3, 4},           // 0x7C *NOP a,x - unofficial
        {ABS_INDEX_X, ADC, 3, 4},   // 0x7D ADC a,x
        {ABS_INDEX_X, ROR, 3, 7},   // 0x7E ROR a,x
        {IMP, NOP, 3, 7},           // 0x7F *RRA a,x - unofficial
    },
    // Row 8: 0x80 - 0x8F
    {
        {IMP, NOP, 2, 2},         // 0x80 *NOP # - unofficial
        {ZP_INDX_IND, STA, 2, 6}, // 0x81 STA (zp,x)
        {IMP, NOP, 2, 2},         // 0x82 *NOP # - unofficial
        {IMP, NOP, 2, 6},         // 0x83 *SAX (zp,x) - unofficial
        {ZP, STY, 2, 3},          // 0x84 STY zp
        {ZP, STA, 2, 3},          // 0x85 STA zp
        {ZP, STX, 2, 3},          // 0x86 STX zp
        {IMP, NOP, 2, 3},         // 0x87 *SAX zp - unofficial
        {IMP, DEY, 1, 2},         // 0x88 DEY
        {IMP, NOP, 2, 2},         // 0x89 *NOP # - unofficial
        {IMP, TXA, 1, 2},         // 0x8A TXA
        {IMP, NOP, 2, 2},         // 0x8B *XAA # - unofficial (unstable)
        {ABS_A, STY, 3, 4},       // 0x8C STY a
        {ABS_A, STA, 3, 4},       // 0x8D STA a
        {ABS_A, STX, 3, 4},       // 0x8E STX a
        {IMP, NOP, 3, 4},         // 0x8F *SAX a - unofficial
    },
    // Row 9: 0x90 - 0x9F
    {
        {PCR, BCC, 2, 2},           // 0x90 BCC
        {ZP_IND_INDX_Y, STA, 2, 6}, // 0x91 STA (zp),y
        {ZP_IND, STA, 2, 5},        // 0x92 STA (zp)
        {IMP, NOP, 2, 6},           // 0x93 *AHX (zp),y - unofficial (unstable)
        {ZP_INDX_X, STY, 2, 4},     // 0x94 STY zp,x
        {ZP_INDX_X, STA, 2, 4},     // 0x95 STA zp,x
        {ZP_INDX_Y, STX, 2, 4},     // 0x96 STX zp,y
        {IMP, NOP, 2, 4},           // 0x97 *SAX zp,y - unofficial
        {IMP, TYA, 1, 2},           // 0x98 TYA
        {ABS_INDEX_Y, STA, 3, 5},   // 0x99 STA a,y
        {IMP, TXS, 1, 2},           // 0x9A TXS
        {IMP, NOP, 3, 5},           // 0x9B *TAS a,y - unofficial (unstable)
        {IMP, NOP, 3, 5},           // 0x9C *SHY a,x - unofficial (unstable)
        {ABS_INDEX_X, STA, 3, 5},   // 0x9D STA a,x
        {IMP, NOP, 3, 5},           // 0x9E *SHX a,y - unofficial (unstable)
        {IMP, NOP, 3, 5},           // 0x9F *AHX a,y - unofficial (unstable)
    },
    // Row A: 0xA0 - 0xAF
    {
        {IMM, LDY, 2, 2},         // 0xA0 LDY #
        {ZP_INDX_IND, LDA, 2, 6}, // 0xA1 LDA (zp,x)
        {IMM, LDX, 2, 2},         // 0xA2 LDX #
        {IMP, NOP, 2, 6},         // 0xA3 *LAX (zp,x) - unofficial
        {ZP, LDY, 2, 3},          // 0xA4 LDY zp
        {ZP, LDA, 2, 3},          // 0xA5 LDA zp
        {ZP, LDX, 2, 3},          // 0xA6 LDX zp
        {IMP, NOP, 2, 3},         // 0xA7 *LAX zp - unofficial
        {IMP, TAY, 1, 2},         // 0xA8 TAY
        {IMM, LDA, 2, 2},         // 0xA9 LDA #
        {IMP, TAX, 1, 2},         // 0xAA TAX
        {IMP, NOP, 2, 2},         // 0xAB *LAX # - unofficial (unstable)
        {ABS_A, LDY, 3, 4},       // 0xAC LDY a
        {ABS_A, LDA, 3, 4},       // 0xAD LDA a
        {ABS_A, LDX, 3, 4},       // 0xAE LDX a
        {IMP, NOP, 3, 4},         // 0xAF *LAX a - unofficial
    },
    // Row B: 0xB0 - 0xBF
    {
        {PCR, BCS, 2, 2},           // 0xB0 BCS
        {ZP_IND_INDX_Y, LDA, 2, 5}, // 0xB1 LDA (zp),y
        {ZP_IND, LDA, 2, 5},        // 0xB2 LDA (zp)
        {IMP, NOP, 2, 5},           // 0xB3 *LAX (zp),y - unofficial
        {ZP_INDX_X, LDY, 2, 4},     // 0xB4 LDY zp,x
        {ZP_INDX_X, LDA, 2, 4},     // 0xB5 LDA zp,x
        {ZP_INDX_Y, LDX, 2, 4},     // 0xB6 LDX zp,y
        {IMP, NOP, 2, 4},           // 0xB7 *LAX zp,y - unofficial
        {IMP, CLV, 1, 2},           // 0xB8 CLV
        {ABS_INDEX_Y, LDA, 3, 4},   // 0xB9 LDA a,y
        {IMP, TSX, 1, 2},           // 0xBA TSX
        {IMP, NOP, 3, 4},           // 0xBB *LAS a,y - unofficial
        {ABS_INDEX_X, LDY, 3, 4},   // 0xBC LDY a,x
        {ABS_INDEX_X, LDA, 3, 4},   // 0xBD LDA a,x
        {ABS_INDEX_Y, LDX, 3, 4},   // 0xBE LDX a,y
        {IMP, NOP, 3, 4},           // 0xBF *LAX a,y - unofficial
    },
    // Row C: 0xC0 - 0xCF
    {
        {IMM, CPY, 2, 2},         // 0xC0 CPY #
        {ZP_INDX_IND, CMP, 2, 6}, // 0xC1 CMP (zp,x)
        {IMP, NOP, 2, 2},         // 0xC2 *NOP # - unofficial
        {IMP, NOP, 2, 8},         // 0xC3 *DCP (zp,x) - unofficial
        {ZP, CPY, 2, 3},          // 0xC4 CPY zp
        {ZP, CMP, 2, 3},          // 0xC5 CMP zp
        {ZP, DEC, 2, 5},          // 0xC6 DEC zp
        {IMP, NOP, 2, 5},         // 0xC7 *DCP zp - unofficial
        {IMP, INY, 1, 2},         // 0xC8 INY
        {IMM, CMP, 2, 2},         // 0xC9 CMP #
        {IMP, DEX, 1, 2},         // 0xCA DEX
        {IMP, NOP, 2, 2},         // 0xCB *AXS # - unofficial
        {ABS_A, CPY, 3, 4},       // 0xCC CPY a
        {ABS_A, CMP, 3, 4},       // 0xCD CMP a
        {ABS_A, DEC, 3, 6},       // 0xCE DEC a
        {IMP, NOP, 3, 6},         // 0xCF *DCP a - unofficial
    },
    // Row D: 0xD0 - 0xDF
    {
        {PCR, BNE, 2, 2},           // 0xD0 BNE
        {ZP_IND_INDX_Y, CMP, 2, 5}, // 0xD1 CMP (zp),y
        {ZP_IND, CMP, 2, 5},        // 0xD2 CMP (zp)
        {IMP, NOP, 2, 8},           // 0xD3 *DCP (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0xD4 *NOP zp,x - unofficial
        {ZP_INDX_X, CMP, 2, 4},     // 0xD5 CMP zp,x
        {ZP_INDX_X, DEC, 2, 6},     // 0xD6 DEC zp,x
        {IMP, NOP, 2, 6},           // 0xD7 *DCP zp,x - unofficial
        {IMP, CLD, 1, 2},           // 0xD8 CLD
        {ABS_INDEX_Y, CMP, 3, 4},   // 0xD9 CMP a,y
        {IMP, NOP, 1, 2},           // 0xDA *NOP - unofficial
        {IMP, NOP, 3, 7},           // 0xDB *DCP a,y - unofficial
        {IMP, NOP, 3, 4},           // 0xDC *NOP a,x - unofficial
        {ABS_INDEX_X, CMP, 3, 4},   // 0xDD CMP a,x
        {ABS_INDEX_X, DEC, 3, 7},   // 0xDE DEC a,x
        {IMP, NOP, 3, 7},           // 0xDF *DCP a,x - unofficial
    },
    // Row E: 0xE0 - 0xEF
    {
        {IMM, CPX, 2, 2},         // 0xE0 CPX #
        {ZP_INDX_IND, SBC, 2, 6}, // 0xE1 SBC (zp,x)
        {IMP, NOP, 2, 2},         // 0xE2 *NOP # - unofficial
        {IMP, NOP, 2, 8},         // 0xE3 *ISC (zp,x) - unofficial
        {ZP, CPX, 2, 3},          // 0xE4 CPX zp
        {ZP, SBC, 2, 3},          // 0xE5 SBC zp
        {ZP, INC, 2, 5},          // 0xE6 INC zp
        {IMP, NOP, 2, 5},         // 0xE7 *ISC zp - unofficial
        {IMP, INX, 1, 2},         // 0xE8 INX
        {IMM, SBC, 2, 2},         // 0xE9 SBC #
        {IMP, NOP, 1, 2},         // 0xEA NOP (official)
        {IMP, NOP, 2, 2},         // 0xEB *SBC # - unofficial
        {ABS_A, CPX, 3, 4},       // 0xEC CPX a
        {ABS_A, SBC, 3, 4},       // 0xED SBC a
        {ABS_A, INC, 3, 6},       // 0xEE INC a
        {IMP, NOP, 3, 6},         // 0xEF *ISC a - unofficial
    },
    // Row F: 0xF0 - 0xFF
    {
        {PCR, BEQ, 2, 2},           // 0xF0 BEQ
        {ZP_IND_INDX_Y, SBC, 2, 5}, // 0xF1 SBC (zp),y
        {ZP_IND, SBC, 2, 5},        // 0xF2 SBC (zp)
        {IMP, NOP, 2, 8},           // 0xF3 *ISC (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0xF4 *NOP zp,x - unofficial
        {ZP_INDX_X, SBC, 2, 4},     // 0xF5 SBC zp,x
        {ZP_INDX_X, INC, 2, 6},     // 0xF6 INC zp,x
        {IMP, NOP, 2, 6},           // 0xF7 *ISC zp,x - unofficial
        {IMP, SED, 1, 2},           // 0xF8 SED
        {ABS_INDEX_Y, SBC, 3, 4},   // 0xF9 SBC a,y
        {IMP, NOP, 1, 2},           // 0xFA *NOP - unofficial
        {IMP, NOP, 3, 7},           // 0xFB *ISC a,y - unofficial
        {IMP, NOP, 3, 4},           // 0xFC *NOP a,x - unofficial
        {ABS_INDEX_X, SBC, 3, 4},   // 0xFD SBC a,x
        {ABS_INDEX_X, INC, 3, 7},   // 0xFE INC a,x
        {IMP, NOP, 3, 7},           // 0xFF *ISC a,x - unofficial
    }};

ExecutionInfo getExecutionInfo(unsigned char opCode)
{
    unsigned char lower = opCode & 0b00001111;
    unsigned char upper = opCode >> 4;
    lastInstruction = lookUpTable[upper][lower];
    return lastInstruction;
}
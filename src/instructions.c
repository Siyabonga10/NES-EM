#include "instructions.h"
#include "bus.h"
#include "addressingModes.h"
#include <stdlib.h>

static ExecutionInfo lastInstruction = (ExecutionInfo){.addressingMode = NULL, .executor = NULL, .instructionSize = 0, .clockCycles = 0};

ExecutionInfo getExecutionInfo(unsigned char opCode) {

    lastInstruction = (ExecutionInfo){.addressingMode = NULL, .executor = NULL, .instructionSize = 0, .clockCycles = 0};
    return (ExecutionInfo){.addressingMode = NULL, .executor = NULL, .instructionSize = 0, .clockCycles = 0};
}


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

unsigned char ORA(int operandAddr){
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
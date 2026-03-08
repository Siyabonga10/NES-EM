#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H
#include "ExecutionInfo.h"

ExecutionInfo getExecutionInfo(unsigned char opCode);

// Define all instructions
// General format is to take in the address to the operand, compute the result, potentially having side effects, return the result just in case
unsigned char ADC(int operandAddr);
unsigned char AND(int operandAddr);
unsigned char ASL(int operandAddr);
unsigned char BCC(int operandAddr);
unsigned char BCS(int operandAddr);
unsigned char BEQ(int operandAddr);
unsigned char BIT(int operandAddr);
unsigned char BMI(int operandAddr);
unsigned char BNE(int operandAddr);
unsigned char BPL(int operandAddr);
unsigned char BVC(int operandAddr);
unsigned char BRK(int operandAddr);
unsigned char BVS(int operandAddr);
unsigned char CLC(int operandAddr);
unsigned char CLD(int operandAddr);
unsigned char CLI(int operandAddr);
unsigned char CLV(int operandAddr);
unsigned char CMP(int operandAddr);
unsigned char CPX(int operandAddr);
unsigned char CPY(int operandAddr);
unsigned char DEC(int operandAddr);
unsigned char DEX(int operandAddr);
unsigned char DEY(int operandAddr);
unsigned char EOR(int operandAddr);
unsigned char INC(int operandAddr);
unsigned char INX(int operandAddr);
unsigned char INY(int operandAddr);
unsigned char JMP(int operandAddr);
unsigned char JSR(int operandAddr);
unsigned char LDA(int operandAddr);
unsigned char LDX(int operandAddr);
unsigned char LDY(int operandAddr);
unsigned char LSR(int operandAddr);
unsigned char NOP(int operandAddr);
unsigned char ORA(int operandAddr);
unsigned char PHA(int operandAddr);
unsigned char PHP(int operandAddr);
unsigned char PLA(int operandAddr);
unsigned char PLP(int operandAddr);
unsigned char ROL(int operandAddr);
unsigned char ROR(int operandAddr);
unsigned char RTI(int operandAddr);
unsigned char RTS(int operandAddr);
unsigned char SBC(int operandAddr);
unsigned char SEC(int operandAddr);
unsigned char SED(int operandAddr);
unsigned char SEI(int operandAddr);
unsigned char STA(int operandAddr);
unsigned char STX(int operandAddr);
unsigned char STY(int operandAddr);
unsigned char TAX(int operandAddr);
unsigned char TAY(int operandAddr);
unsigned char TSX(int operandAddr);
unsigned char TXA(int operandAddr);
unsigned char TXS(int operandAddr);
unsigned char TYA(int operandAddr);

// Treat the NMI as an instruction
void NMI();

#endif
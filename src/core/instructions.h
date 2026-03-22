#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H
#include "ExecutionInfo.h"
#include <stdbool.h>

ExecutionInfo getExecutionInfo(unsigned char opCode);

// Define all instructions
// General format is to take in the address to the operand, now also a pointer to handle edge cases where an instruction needs extra clock cycles to complete due to some reason, compute the result, potentially having side effects, return the result just in case
unsigned char ADC(int operandAddr, int *additionalClockCycles);
unsigned char AND(int operandAddr, int *additionalClockCycles);
unsigned char ASL(int operandAddr, int *additionalClockCycles);
unsigned char BCC(int operandAddr, int *additionalClockCycles);
unsigned char BCS(int operandAddr, int *additionalClockCycles);
unsigned char BEQ(int operandAddr, int *additionalClockCycles);
unsigned char BIT(int operandAddr, int *additionalClockCycles);
unsigned char BMI(int operandAddr, int *additionalClockCycles);
unsigned char BNE(int operandAddr, int *additionalClockCycles);
unsigned char BPL(int operandAddr, int *additionalClockCycles);
unsigned char BVC(int operandAddr, int *additionalClockCycles);
unsigned char BRK(int operandAddr, int *additionalClockCycles);
unsigned char BVS(int operandAddr, int *additionalClockCycles);
unsigned char CLC(int operandAddr, int *additionalClockCycles);
unsigned char CLD(int operandAddr, int *additionalClockCycles);
unsigned char CLI(int operandAddr, int *additionalClockCycles);
unsigned char CLV(int operandAddr, int *additionalClockCycles);
unsigned char CMP(int operandAddr, int *additionalClockCycles);
unsigned char CPX(int operandAddr, int *additionalClockCycles);
unsigned char CPY(int operandAddr, int *additionalClockCycles);
unsigned char DEC(int operandAddr, int *additionalClockCycles);
unsigned char DEX(int operandAddr, int *additionalClockCycles);
unsigned char DEY(int operandAddr, int *additionalClockCycles);
unsigned char EOR(int operandAddr, int *additionalClockCycles);
unsigned char INC(int operandAddr, int *additionalClockCycles);
unsigned char INX(int operandAddr, int *additionalClockCycles);
unsigned char INY(int operandAddr, int *additionalClockCycles);
unsigned char JMP(int operandAddr, int *additionalClockCycles);
unsigned char JSR(int operandAddr, int *additionalClockCycles);
unsigned char LDA(int operandAddr, int *additionalClockCycles);
unsigned char LDX(int operandAddr, int *additionalClockCycles);
unsigned char LDY(int operandAddr, int *additionalClockCycles);
unsigned char LSR(int operandAddr, int *additionalClockCycles);
unsigned char NOP(int operandAddr, int *additionalClockCycles);
unsigned char ORA(int operandAddr, int *additionalClockCycles);
unsigned char PHA(int operandAddr, int *additionalClockCycles);
unsigned char PHP(int operandAddr, int *additionalClockCycles);
unsigned char PLA(int operandAddr, int *additionalClockCycles);
unsigned char PLP(int operandAddr, int *additionalClockCycles);
unsigned char ROL(int operandAddr, int *additionalClockCycles);
unsigned char ROR(int operandAddr, int *additionalClockCycles);
unsigned char RTI(int operandAddr, int *additionalClockCycles);
unsigned char RTS(int operandAddr, int *additionalClockCycles);
unsigned char SBC(int operandAddr, int *additionalClockCycles);
unsigned char SEC(int operandAddr, int *additionalClockCycles);
unsigned char SED(int operandAddr, int *additionalClockCycles);
unsigned char SEI(int operandAddr, int *additionalClockCycles);
unsigned char STA(int operandAddr, int *additionalClockCycles);
unsigned char STX(int operandAddr, int *additionalClockCycles);
unsigned char STY(int operandAddr, int *additionalClockCycles);
unsigned char TAX(int operandAddr, int *additionalClockCycles);
unsigned char TAY(int operandAddr, int *additionalClockCycles);
unsigned char TSX(int operandAddr, int *additionalClockCycles);
unsigned char TXA(int operandAddr, int *additionalClockCycles);
unsigned char TXS(int operandAddr, int *additionalClockCycles);
unsigned char TYA(int operandAddr, int *additionalClockCycles);

// Treat the NMI as an instruction
void NMI();
void executeNMI();
bool pendingNMI();

#endif
#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H
#include "ExecutionInfo.h"
#include <stdbool.h>

ExecutionInfo getExecutionInfo(unsigned char opCode);

// Define all instructions
// General format is to take in the address to the operand, now also a pointer to handle edge cases where an instruction needs extra clock cycles to complete due to some reason, compute the result, potentially having side effects, return the result just in case
unsigned char ADC(int operandAddr, ExecutionInfo *exInfo);
unsigned char AND(int operandAddr, ExecutionInfo *exInfo);
unsigned char ASL(int operandAddr, ExecutionInfo *exInfo);
unsigned char BCC(int operandAddr, ExecutionInfo *exInfo);
unsigned char BCS(int operandAddr, ExecutionInfo *exInfo);
unsigned char BEQ(int operandAddr, ExecutionInfo *exInfo);
unsigned char BIT(int operandAddr, ExecutionInfo *exInfo);
unsigned char BMI(int operandAddr, ExecutionInfo *exInfo);
unsigned char BNE(int operandAddr, ExecutionInfo *exInfo);
unsigned char BPL(int operandAddr, ExecutionInfo *exInfo);
unsigned char BVC(int operandAddr, ExecutionInfo *exInfo);
unsigned char BRK(int operandAddr, ExecutionInfo *exInfo);
unsigned char BVS(int operandAddr, ExecutionInfo *exInfo);
unsigned char CLC(int operandAddr, ExecutionInfo *exInfo);
unsigned char CLD(int operandAddr, ExecutionInfo *exInfo);
unsigned char CLI(int operandAddr, ExecutionInfo *exInfo);
unsigned char CLV(int operandAddr, ExecutionInfo *exInfo);
unsigned char CMP(int operandAddr, ExecutionInfo *exInfo);
unsigned char CPX(int operandAddr, ExecutionInfo *exInfo);
unsigned char CPY(int operandAddr, ExecutionInfo *exInfo);
unsigned char DEC(int operandAddr, ExecutionInfo *exInfo);
unsigned char DEX(int operandAddr, ExecutionInfo *exInfo);
unsigned char DEY(int operandAddr, ExecutionInfo *exInfo);
unsigned char EOR(int operandAddr, ExecutionInfo *exInfo);
unsigned char INC(int operandAddr, ExecutionInfo *exInfo);
unsigned char INX(int operandAddr, ExecutionInfo *exInfo);
unsigned char INY(int operandAddr, ExecutionInfo *exInfo);
unsigned char JMP(int operandAddr, ExecutionInfo *exInfo);
unsigned char JSR(int operandAddr, ExecutionInfo *exInfo);
unsigned char LDA(int operandAddr, ExecutionInfo *exInfo);
unsigned char LDX(int operandAddr, ExecutionInfo *exInfo);
unsigned char LDY(int operandAddr, ExecutionInfo *exInfo);
unsigned char LSR(int operandAddr, ExecutionInfo *exInfo);
unsigned char NOP(int operandAddr, ExecutionInfo *exInfo);
unsigned char ORA(int operandAddr, ExecutionInfo *exInfo);
unsigned char PHA(int operandAddr, ExecutionInfo *exInfo);
unsigned char PHP(int operandAddr, ExecutionInfo *exInfo);
unsigned char PLA(int operandAddr, ExecutionInfo *exInfo);
unsigned char PLP(int operandAddr, ExecutionInfo *exInfo);
unsigned char ROL(int operandAddr, ExecutionInfo *exInfo);
unsigned char ROR(int operandAddr, ExecutionInfo *exInfo);
unsigned char RTI(int operandAddr, ExecutionInfo *exInfo);
unsigned char RTS(int operandAddr, ExecutionInfo *exInfo);
unsigned char SBC(int operandAddr, ExecutionInfo *exInfo);
unsigned char SEC(int operandAddr, ExecutionInfo *exInfo);
unsigned char SED(int operandAddr, ExecutionInfo *exInfo);
unsigned char SEI(int operandAddr, ExecutionInfo *exInfo);
unsigned char STA(int operandAddr, ExecutionInfo *exInfo);
unsigned char STX(int operandAddr, ExecutionInfo *exInfo);
unsigned char STY(int operandAddr, ExecutionInfo *exInfo);
unsigned char TAX(int operandAddr, ExecutionInfo *exInfo);
unsigned char TAY(int operandAddr, ExecutionInfo *exInfo);
unsigned char TSX(int operandAddr, ExecutionInfo *exInfo);
unsigned char TXA(int operandAddr, ExecutionInfo *exInfo);
unsigned char TXS(int operandAddr, ExecutionInfo *exInfo);
unsigned char TYA(int operandAddr, ExecutionInfo *exInfo);

// Treat the NMI as an instruction
void NMI();
void triggerDelayedNMI();
void cpu_instruction_completed();
void executeNMI();
bool pendingNMI();

// IRQ handling
void triggerIRQ();
void executeIRQ();
bool pendingIRQ();
void clearPendingIRQ();

#endif
#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H
#include "ExecutionInfo.h"
#include <stdbool.h>

ExecutionInfo getExecutionInfo(unsigned char opCode);

// Define all instructions
// General format is to take in the address to the operand, now also a pointer to handle edge cases where an instruction needs extra clock cycles to complete due to some reason, compute the result, potentially having side effects, return the result just in case
unsigned char ADC(ExecutionInfo *exInfo);
unsigned char AND(ExecutionInfo *exInfo);
unsigned char ASL(ExecutionInfo *exInfo);
unsigned char BCC(ExecutionInfo *exInfo);
unsigned char BCS(ExecutionInfo *exInfo);
unsigned char BEQ(ExecutionInfo *exInfo);
unsigned char BIT(ExecutionInfo *exInfo);
unsigned char BMI(ExecutionInfo *exInfo);
unsigned char BNE(ExecutionInfo *exInfo);
unsigned char BPL(ExecutionInfo *exInfo);
unsigned char BVC(ExecutionInfo *exInfo);
unsigned char BRK(ExecutionInfo *exInfo);
unsigned char BVS(ExecutionInfo *exInfo);
unsigned char CLC(ExecutionInfo *exInfo);
unsigned char CLD(ExecutionInfo *exInfo);
unsigned char CLI(ExecutionInfo *exInfo);
unsigned char CLV(ExecutionInfo *exInfo);
unsigned char CMP(ExecutionInfo *exInfo);
unsigned char CPX(ExecutionInfo *exInfo);
unsigned char CPY(ExecutionInfo *exInfo);
unsigned char DEC(ExecutionInfo *exInfo);
unsigned char DEX(ExecutionInfo *exInfo);
unsigned char DEY(ExecutionInfo *exInfo);
unsigned char EOR(ExecutionInfo *exInfo);
unsigned char INC(ExecutionInfo *exInfo);
unsigned char INX(ExecutionInfo *exInfo);
unsigned char INY(ExecutionInfo *exInfo);
unsigned char JMP(ExecutionInfo *exInfo);
unsigned char JSR(ExecutionInfo *exInfo);
unsigned char LDA(ExecutionInfo *exInfo);
unsigned char LDX(ExecutionInfo *exInfo);
unsigned char LDY(ExecutionInfo *exInfo);
unsigned char LSR(ExecutionInfo *exInfo);
unsigned char NOP(ExecutionInfo *exInfo);
unsigned char ORA(ExecutionInfo *exInfo);
unsigned char PHA(ExecutionInfo *exInfo);
unsigned char PHP(ExecutionInfo *exInfo);
unsigned char PLA(ExecutionInfo *exInfo);
unsigned char PLP(ExecutionInfo *exInfo);
unsigned char ROL(ExecutionInfo *exInfo);
unsigned char ROR(ExecutionInfo *exInfo);
unsigned char RTI(ExecutionInfo *exInfo);
unsigned char RTS(ExecutionInfo *exInfo);
unsigned char SBC(ExecutionInfo *exInfo);
unsigned char SEC(ExecutionInfo *exInfo);
unsigned char SED(ExecutionInfo *exInfo);
unsigned char SEI(ExecutionInfo *exInfo);
unsigned char STA(ExecutionInfo *exInfo);
unsigned char STX(ExecutionInfo *exInfo);
unsigned char STY(ExecutionInfo *exInfo);
unsigned char TAX(ExecutionInfo *exInfo);
unsigned char TAY(ExecutionInfo *exInfo);
unsigned char TSX(ExecutionInfo *exInfo);
unsigned char TXA(ExecutionInfo *exInfo);
unsigned char TXS(ExecutionInfo *exInfo);
unsigned char TYA(ExecutionInfo *exInfo);

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
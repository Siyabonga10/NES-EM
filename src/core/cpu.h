#ifndef CPU_H
#define CPU_H
#include <stdbool.h>
#include <stdlib.h>
#include "ExecutionInfo.h"
#include "ControllerKeyStates.h"

void bootCPU();
void tickCPU(ControllerKeyStates *keyState);
ExecutionInfo getNextInstruction();
void shutdownCPU();

int executeInstruction(ExecutionInfo exInfo);

#endif
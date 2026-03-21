#ifndef CPU_H
#define CPU_H
#include <stdbool.h>
#include <stdlib.h>
#include "ExecutionInfo.h"

void bootCPU();
void runCPU();
ExecutionInfo getNextInstruction();
void shutdownCPU();

int executeInstruction(ExecutionInfo exInfo);

#endif
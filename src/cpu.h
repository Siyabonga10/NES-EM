#ifndef CPU_H
#define CPU_H
#include <stdbool.h>
#include <stdlib.h>
#include "ExecutionInfo.h"


void bootCPU(bool showWindow);
void runCPU();
ExecutionInfo getNextInstruction();
void shutdownCPU();

void executeInstruction(ExecutionInfo exInfo);


#endif
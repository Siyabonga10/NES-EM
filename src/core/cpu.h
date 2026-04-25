#ifndef CPU_H
#define CPU_H
#include <stdbool.h>
#include <stdlib.h>
#include "ExecutionInfo.h"
#include "ControllerKeyStates.h"
#include "frameData.h"

void boot_cpu();
FrameData *tick_cpu(ControllerKeyStates *keyState);
ExecutionInfo get_next_instruction();
void shutdown_cpu();

int execute_instruction(ExecutionInfo exInfo);

#endif
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

void trace_dump_ringbuffer(void);
void trace_set_enabled(bool on);

#endif
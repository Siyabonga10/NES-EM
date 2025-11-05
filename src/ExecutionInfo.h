#ifndef EXECUTION_INFO_H
#define EXECUTION_INFO_H

typedef struct {
    int (*addressingMode) (int);
    unsigned char (*executor) (int);
    int instructionSize;
    int clockCycles;
} ExecutionInfo;

#endif 
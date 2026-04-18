#ifndef EXECUTION_INFO_H
#define EXECUTION_INFO_H

typedef struct ExecutionInfo ExecutionInfo;

struct ExecutionInfo
{
    int (*addressingMode)(int);
    unsigned char (*executor)(ExecutionInfo *);
    int instructionSize;
    int clockCycles;
};

#endif
#ifndef EXECUTION_INFO_H
#define EXECUTION_INFO_H

typedef struct ExecutionInfo ExecutionInfo;

struct ExecutionInfo
{
    int (*addressing_mode)(int);
    unsigned char (*executor)(ExecutionInfo *);
    int instruction_size;
    int clock_cycles;
};

#endif
#include "mapper.h"
#include <assert.h>

int M000(int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int mapped = addr - 0x8000;
    mapped %= 0x4000;

    return mapped + 0x2000;
}
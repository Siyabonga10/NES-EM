#include "mapper.h"
#include <assert.h>

int M000(int addr)
{
    assert(0x8000 <= addr && addr <= 0xFFFF);
    return addr - 0x8000;
}
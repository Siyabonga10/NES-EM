#include "mapper.h"
#include "cartriadge.h"
#include <assert.h>

int M000(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;

    int mapped = addr - 0x8000;
    if (cart->pg_rom_size < 0x4000)
    {
        mapped %= 0x4000;
    }

    return mapped + 0x2000;
}
#include <stdlib.h>

void *nes_alloc(size_t byte_count)
{
  return malloc(byte_count);
}

void *nes_dealloc(void *mem)
{
  free(mem);
}
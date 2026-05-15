#include "crc32.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CRC32_POLY 0xEDB88320U

uint32_t extract_value(const uint32_t *bits, uint32_t index) {
  uint32_t val;
  memcpy(&val, bits + index, sizeof(uint32_t));
  return val;
}

void print_32_bit(uint32_t value) {
  for (int32_t i = 31; i >= 0; i--) {
    if ((i + 1) % 4 == 0)
      printf(" ");
    printf("%i", (value >> i) & 1);
  }
  printf("\n");
}

uint32_t crc32(const void *data, size_t len) {
  if (len == 0)
    return 0;
  const uint8_t *bytes     = (const uint8_t *)data;
  uint32_t       shift_reg = 0xFFFFFFFFU;
  for (size_t i = 0; i < len; i++) {
    shift_reg ^= bytes[i];
    for (uint32_t j = 0; j < 8; j++) {
      uint32_t lsb = shift_reg & 1U;
      shift_reg >>= 1;
      if (lsb)
        shift_reg ^= CRC32_POLY;
    }
  }
  shift_reg ^= 0xFFFFFFFFU;
  return shift_reg;
}
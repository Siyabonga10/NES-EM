#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stddef.h>

uint32_t crc32(const void *data, size_t len);

#endif

#ifndef MAPPER_REGISTER
#define MAPPER_REGISTER
#include "mapper_mount.h"
#include "../rom_loader.h"
#include <stdlib.h>

#ifdef _MSC_VER
    #pragma section(".CRT$XCZ",read)
    #define REGISTER_MAPPER(mapper, idx)                                    \
        static void register_mapper_##mapper(void) { register_mapper(mapper, idx); } \
        __declspec(allocate(".CRT$XCZ")) void (*_reg_##mapper)(void) = register_mapper_##mapper;
#else
    #define REGISTER_MAPPER(mapper, idx)              \
        __attribute__((constructor))                  \
        static void register_mapper_##mapper(void) {  \
            register_mapper(mapper, idx);             \
        }
#endif

#endif

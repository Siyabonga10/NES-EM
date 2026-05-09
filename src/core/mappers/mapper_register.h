#ifndef MAPPER_REGISTER
#define MAPPER_REGISTER
#include "mapper_mount.h"
#include "../rom_loader.h"
#include <stdlib.h>

#define REGISTER_MAPPER(mapper, idx)              \
    __attribute__((constructor))                  \
    static void register_mapper_##mapper(void) {  \
        register_mapper(mapper, idx);             \
    }

#endif
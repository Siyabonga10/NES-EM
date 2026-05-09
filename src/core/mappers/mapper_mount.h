#ifndef MAPPER_MOUNT
#define MAPPER_MOUNT
#include "../cartriadge.h"
#include "../ines_one_rom_info.h"

typedef void (*mount_mapper_to_catridge)(Cartriadge* cart, iNesOneRomInfo cart_info);

#endif
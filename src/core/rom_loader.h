#ifndef ROM_LOADER
#define ROM_LOADER
#include "./mappers/mapper_mount.h"
#include <stdlib.h>

void register_mapper(mount_mapper_to_catridge mapper, size_t index);
void load_cartridge(char *filePath, Cartriadge *cart);
void load_cartridge_and_connect_to_bus(char *contents, int lenContents);
int load_cartridge_from_memory(unsigned char *data, int len, Cartriadge *cart);

#endif
#ifndef CARTRIADGE_H
#define CARTRIADGE_H

typedef struct {
    unsigned char* mem;
    int (*mapper)(int);
    int size;
    int pg_rom_size;
    int ch_ram_size;
    int mirroring_mode;
} Cartriadge;

void loadCartriadge(char* filePath, Cartriadge* cart);

#endif
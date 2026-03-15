#ifndef CARTRIADGE_H
#define CARTRIADGE_H

typedef struct Cartriadge
{
    unsigned char *mem;
    int (*mapper)(struct Cartriadge *, int);
    int size;
    int pg_rom_size;
    int ch_ram_size;
    int mirroring_mode;
} Cartriadge;

void loadCartriadge(char *filePath, Cartriadge *cart);

#endif
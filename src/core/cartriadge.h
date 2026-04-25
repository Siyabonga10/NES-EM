#ifndef CARTRIADGE_H
#define CARTRIADGE_H

typedef struct Cartriadge
{
    unsigned char *mem;
    unsigned char *chr_ram;
    int (*mapper)(struct Cartriadge *, int);
    int (*ppu_mapper)(struct Cartriadge *, int);
    void (*cart_writer)(struct Cartriadge *, int, unsigned char);
    void (*scanline_tick)(struct Cartriadge *);
    int size;
    int pg_rom_size;
    int ch_ram_size;
    int mirroring_mode;
} Cartriadge;

void load_cartridge(char *filePath, Cartriadge *cart);
void load_cartridge_and_connect_to_bus(char *contents, int lenContents);

#endif
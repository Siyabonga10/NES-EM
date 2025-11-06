#include "cartriadge.h"
#include "mapper.h"
#include <stdio.h>
#include <string.h>

void loadCartriadge(char *filePath, Cartriadge* cart)
{
    FILE* fptr = fopen(filePath, 'r');
    if(fptr == NULL)
    {
        printf("Could not load cartridge\n");
        return;
    }
    char* buffer = (char*)malloc(0xFFFFF); // leave it as 1mb for now
    if(!fgets(buffer, 0xFFFFF, fptr))
    {
        printf("Error while reading cartriadge\n");
        free(buffer);
        return;
    }
    // Assuming trainer doesnt exist for simplicity
    int pgRomSize = buffer[4]; // in 16kB units
    int chrRomSize = buffer[5]; // in 8kB units
    cart->mem = malloc(0xFFFF - 0x6000); // TODO: Size may be larger than this
    memset(cart->mem, 0, 0xFFFF - 0x6000);
    int offset = 0x2000;
    memcpy(cart->mem, buffer + offset, pgRomSize * 0x4000);
    offset += pgRomSize * 0x4000;
    if(pgRomSize == 1)
    {
        memcpy(cart->mem, buffer + offset, pgRomSize * 0x4000);
        offset += 0x4000;
    }
    memcpy(cart->mem, buffer + offset, chrRomSize * 0x2000);
    cart->mapper = M000;
    free(buffer);
}

static int isFlagSet(int position, unsigned char byte) {
    return byte & (1 << position);
}
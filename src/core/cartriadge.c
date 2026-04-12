#include "cartriadge.h"
#include "mappers/mappers.h"
#include "bus.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void loadCartriadge(char *filePath, Cartriadge *cart)
{
    FILE *fptr = fopen(filePath, "rb");
    if (fptr == NULL)
    {
        printf("Could not load cartridge\n");
        return;
    }

    // Read header first (16 bytes)
    unsigned char header[16];
    int readSize = fread(header, 1, 16, fptr);
    if (readSize != 16)
    {
        printf("Error reading NES header, read %i bytes\n", readSize);
        fclose(fptr);
        return;
    }

    // Verify NES header magic number ("NES" followed by MS-DOS EOF)
    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A)
    {
        printf("Invalid NES ROM format\n");
        fclose(fptr);
        return;
    }

    // Get sizes from header
    int pgRomSize = header[4];  // Program ROM size in 16KB units
    int chrRomSize = header[5]; // Character ROM size in 8KB units
    int flags6 = header[6];     // Mapper, mirroring, battery, trainer
    int flags7 = header[7];     // Mapper, VS/Playchoice, NES 2.0
    cart->mirroring_mode = flags6 & 1;
    cart->pg_rom_size = 0x4000 * pgRomSize;
    // Extract mapper number
    int mapperId = (flags7 & 0xF0) | (flags6 >> 4);

    // Check for trainer (we'll skip it if present)
    int hasTrainer = flags6 & 0x04;
    if (hasTrainer)
    {
        printf("Trainer section detected on catriadge.\n");
        fseek(fptr, 512, SEEK_CUR); // Skip trainer
    }

    // Calculate total required memory
    // PRG ROM (16KB units) + CHR ROM (8KB units)
    int totalSize = 0x2000 + (pgRomSize * 0x4000) + (chrRomSize * 0x2000);
    cart->mem = malloc(totalSize);
    cart->size = totalSize;
    cart->chr_ram = NULL;
    cart->ch_ram_size = 0;
    cart->scanlineTick = NULL;

    if (cart->mem == NULL)
    {
        printf("Failed to allocate memory for cartridge\n");
        fclose(fptr);
        return;
    }

    // Read PRG-ROM
    if (fread(cart->mem + 0x2000, 1, pgRomSize * 0x4000, fptr) != pgRomSize * 0x4000)
    {
        printf("Error reading PRG-ROM\n");
        free(cart->mem);
        fclose(fptr);
        return;
    }

    // Read CHR-ROM if present
    if (chrRomSize > 0)
    {
        if (fread(cart->mem + 0x2000 + (pgRomSize * 0x4000), 1, chrRomSize * 0x2000, fptr) != chrRomSize * 0x2000)
        {
            printf("Error reading CHR-ROM\n");
            free(cart->mem);
            fclose(fptr);
            return;
        }
    }
    
    if (chrRomSize == 0) {
        cart->chr_ram = malloc(0x2000);
        memset(cart->chr_ram, 0, 0x2000);
        cart->ch_ram_size = 0x2000;
    }

    // Set mapper based on ID (currently only supports mapper 000)
    if (mapperId == 0)
    {
        cart->mapper = M000;
        cart->ppuMapper = M000_PPU;
        cart->cartWriter = NO_WRITE;
    }
    else if (mapperId == 1)
    {
        cart->mapper = M001;
        cart->ppuMapper = M001_PPU;
        cart->cartWriter = M001_Write;
    }
    else if (mapperId == 2)
    {
        cart->mapper = M002;
        cart->ppuMapper = M002_PPU;
        cart->cartWriter = M002_Write;
    }
    else if (mapperId == 3)
    {
        cart->mapper = M003;
        cart->ppuMapper = M003_PPU;
        cart->cartWriter = M003_Write;
    }
    else if (mapperId == 4)
    {
        cart->mapper = M004;
        cart->ppuMapper = M004_PPU;
        cart->cartWriter = M004_Write;
        cart->scanlineTick = M004_ScanlineTick;
    }
    else if (mapperId == 5)
    {
        cart->mapper = M005;
        cart->ppuMapper = M005_PPU;
        cart->cartWriter = M005_Write;
    }
    else if (mapperId == 6)
    {
        cart->mapper = M006;
        cart->ppuMapper = M006_PPU;
        cart->cartWriter = M006_Write;
    }
    else if (mapperId == 7)
    {
        cart->mapper = M007;
        cart->ppuMapper = M007_PPU;
        cart->cartWriter = M007_Write;
    }
    else if (mapperId == 23)
    {
        cart->mapper = M023;
        cart->ppuMapper = M023_PPU;
        cart->cartWriter = M023_Write;
    }
    else if (mapperId == 66)
    {
        cart->mapper = M066;
        cart->ppuMapper = M066_PPU;
        cart->cartWriter = M066_Write;
    }
    else if (mapperId == 11)
    {
        cart->mapper = M011;
        cart->ppuMapper = M011_PPU;
        cart->cartWriter = M011_Write;
    }
    else if (mapperId == 34)
    {
        cart->mapper = M034;
        cart->ppuMapper = M034_PPU;
        cart->cartWriter = M034_Write;
    }
    else
    {
        printf("Warning: Unsupported mapper %d, defaulting to NROM (000)\n", mapperId);
        cart->mapper = M000;
        cart->ppuMapper = M000_PPU;
        cart->cartWriter = NO_WRITE;
    }
    printf("Successfully loaded cartridge: %s\n", filePath);
    printf("PRG-ROM: %dKB\n", pgRomSize * 16);
    printf("CHR-ROM: %dKB\n", chrRomSize * 8);
    printf("Mapper: %d\n", mapperId);

    fclose(fptr);
}

void loadCartriadgeAndConnectToBus(char *contents, int lenContents)
{
    Cartriadge *testCartriadge = malloc(sizeof(Cartriadge));
    FILE *f = fopen("/tmp/cart.bin", "wb");
    fwrite(contents, 1, lenContents, f);
    fclose(f);
    loadCartriadge("/tmp/cart.bin", testCartriadge);
    connectCartriadgeToBus(testCartriadge);
    remove("/tmp/cart.bin");
}

static int isFlagSet(int position, unsigned char byte)
{
    return byte & (1 << position);
}

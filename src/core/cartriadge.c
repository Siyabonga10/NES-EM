#include "cartriadge.h"
#include "mappers/mappers.h"
#include "bus.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static inline void free_cart_and_close_file_ptr_after_error(Cartriadge *cart, FILE *fptr, const char *error_msg)
{
    printf("%s\n", error_msg);
    free(cart->pg_rom);
    free(cart->ch_rom);
    fclose(fptr);
    return;
}
void load_cartridge(char *filePath, Cartriadge *cart)
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

    cart->pg_rom = malloc(pgRomSize * 0x4000);
    cart->ch_rom = malloc(chrRomSize * 0x2000);
    cart->size = pgRomSize * 0x4000 + chrRomSize * 0x2000;
    cart->chr_ram = NULL;
    cart->ch_ram_size = 0;
    cart->prg_ram = NULL;
    cart->prg_ram_size = 0;
    cart->scanline_tick = NULL;
    cart->cart_writer = NO_WRITE;

    if (fread(cart->pg_rom, sizeof(unsigned char), pgRomSize * 0x4000, fptr) != pgRomSize * 0x4000)
    {
        free_cart_and_close_file_ptr_after_error(cart, fptr, "Error reading PRG-ROM");
        return;
    }

    if (chrRomSize > 0 && fread(cart->ch_rom, sizeof(unsigned char), chrRomSize * 0x2000, fptr) != chrRomSize * 0x2000)
    {
        free_cart_and_close_file_ptr_after_error(cart, fptr, "Error reading CHR-ROM");
        return;
    }

    if (chrRomSize == 0)
    {
        cart->chr_ram = malloc(0x2000);
        memset(cart->chr_ram, 0, 0x2000);
        cart->ch_ram_size = 0x2000;
    }

    // Set mapper based on ID (currently only supports mapper 000)
    if (mapperId == 0)
    {
        cart->mapper = M000;
        cart->ppu_read = M000_PPU;
        cart->cart_writer = NO_WRITE;
        cart->ppu_write = M000_PPU_WRITE;
        cart->pg_rom_bank_size = 0x4000 * pgRomSize;
        cart->pg_rom_bank_count = -1;

        cart->ch_ram_size = 0x2000 * chrRomSize;
        cart->ch_rom_bank_count = -1;
    }
    else if (mapperId == 1)
    {
        cart->mapper = M001;
        cart->ppu_read = M001_PPU;
        cart->cart_writer = M001_Write;
        cart->ppu_write = M001_PPU_WRITE;

        cart->pg_rom_bank_count = pgRomSize;
        cart->ch_rom_bank_count = chrRomSize;
        cart->pg_rom_bank_size = 0x4000; 
        cart->ch_rom_bank_size = chrRomSize == 0 ? 0 : 0x2000;
        cart->prg_ram = malloc(0x2000);
        memset(cart->prg_ram, 0, 0x2000);
        cart->prg_ram_size = 0x2000;
    }
    else if (mapperId == 2)
    {
        cart->mapper = M002;
        cart->ppu_read = M002_PPU;
        cart->cart_writer = M002_Write;
        cart->ppu_write = M002_PPU_WRITE;

        cart->pg_rom_bank_count = pgRomSize;
        cart->ch_rom_bank_count = -1;
        cart->pg_rom_bank_size = 0x4000;
        cart->ch_rom_bank_size = 0x2000;
    }
    else if (mapperId == 3)
    {
        cart->mapper = M003;
        cart->ppu_read = M003_PPU;
        cart->cart_writer = M003_Write;
        cart->ppu_write = M003_PPU_WRITE;

        cart->pg_rom_bank_count = pgRomSize;
        cart->pg_rom_bank_size = 0x4000;

        cart->ch_rom_bank_count = chrRomSize;
        cart->ch_rom_bank_size = 0x2000;
    }
    else if (mapperId == 4)
    {
        cart->mapper = M004;
        cart->ppu_read = M004_PPU;
        cart->cart_writer = M004_Write;
        cart->ppu_write = M004_PPU_WRITE;
        cart->scanline_tick = M004_ScanlineTick;

        cart->pg_rom_bank_count = pgRomSize * 2;
        cart->pg_rom_bank_size = 0x2000;
        cart->ch_rom_bank_count = chrRomSize;
        cart->ch_rom_bank_size = 0x2000;

        cart->prg_ram = malloc(0x2000);
        memset(cart->prg_ram, 0, 0x2000);
        cart->prg_ram_size = 0x2000;
    }
    // else if (mapperId == 5)
    // {
    //     cart->mapper = M005;
    //     cart->ppu_read = M005_PPU;
    //     cart->cart_writer = M005_Write;
    // }
    // else if (mapperId == 6)
    // {
    //     cart->mapper = M006;
    //     cart->ppu_read = M006_PPU;
    //     cart->cart_writer = M006_Write;
    // }
    // else if (mapperId == 7)
    // {
    //     cart->mapper = M007;
    //     cart->ppu_read = M007_PPU;
    //     cart->cart_writer = M007_Write;
    // }
    // else if (mapperId == 23)
    // {
    //     cart->mapper = M023;
    //     cart->ppu_read = M023_PPU;
    //     cart->cart_writer = M023_Write;
    // }
    // else if (mapperId == 66)
    // {
    //     cart->mapper = M066;
    //     cart->ppu_read = M066_PPU;
    //     cart->cart_writer = M066_Write;
    // }
    // else if (mapperId == 11)
    // {
    //     cart->mapper = M011;
    //     cart->ppu_read = M011_PPU;
    //     cart->cart_writer = M011_Write;
    // }
    // else if (mapperId == 34)
    // {
    //     cart->mapper = M034;
    //     cart->ppu_read = M034_PPU;
    //     cart->cart_writer = M034_Write;
    // }
    else
    {
        printf("Warning: Unsupported mapper %d, defaulting to NROM (000)\n", mapperId);
        cart->mapper = M000;
        cart->ppu_read = M000_PPU;
        cart->cart_writer = NO_WRITE;
    }
    printf("Successfully loaded cartridge: %s\n", filePath);
    printf("PRG-ROM: %dKB\n", pgRomSize * 16);
    printf("CHR-ROM: %dKB\n", chrRomSize * 8);
    printf("Mapper: %d\n", mapperId);

    fclose(fptr);
}

void load_cartridge_and_connect_to_bus(char *contents, int lenContents)
{
    Cartriadge *test_cartridge = malloc(sizeof(Cartriadge));
    FILE *f = fopen("/tmp/cart.bin", "wb");
    fwrite(contents, 1, lenContents, f);
    fclose(f);
    load_cartridge("/tmp/cart.bin", test_cartridge);
    connect_cartridge_to_bus(test_cartridge);
    remove("/tmp/cart.bin");
}

static int isFlagSet(int position, unsigned char byte)
{
    return byte & (1 << position);
}

#include "cartriadge.h"
#include "mappers/mappers.h"
#include "bus.h"
#include <stdio.h>
#include "debug_log.h"
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

    fseek(fptr, 0, SEEK_END);
    int rom_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    unsigned char * file_contents = malloc(rom_size);
    fread(file_contents, rom_size, sizeof(unsigned char), fptr);
    load_cartridge_from_memory(file_contents, rom_size, cart);
    fclose(fptr);
}

static inline bool get_rom_info(const char * header, iNesOneRomInfo* info) {
    // Verify NES header magic number ("NES" followed by MS-DOS EOF)
    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A)
    {
        printf("Invalid NES ROM format\n");
        return false;
    }

    info->no_of_pg_rom_banks = header[4];
    info->no_of_ch_rom_banks = header[5];
    info->flags6             = header[6];
    info->flags7             = header[7];

    return true;
}

void common_catridge_setup(Cartriadge* cart, iNesOneRomInfo cart_info, const char* data, int offset) {
    cart->scanline_tick = NULL;
    cart->prg_ram_size  = 0;
    cart->ch_ram_size   = 0;
    cart->cart_writer   = NO_WRITE;
    cart->chr_ram       = NULL;
    cart->prg_ram       = NULL;
    cart->ch_rom        = malloc(cart_info.no_of_ch_rom_banks * 0x2000);
    cart->pg_rom        = malloc(cart_info.no_of_pg_rom_banks * 0x4000);
    cart->size          = cart_info.no_of_pg_rom_banks * 0x4000 + cart_info.no_of_ch_rom_banks * 0x2000;

    memcpy(cart->pg_rom, data + offset, cart_info.no_of_pg_rom_banks * 0x4000);
    offset += cart_info.no_of_pg_rom_banks * 0x4000;

    if (cart_info.no_of_ch_rom_banks > 0)
    {
        memcpy(cart->ch_rom, data + offset, cart_info.no_of_ch_rom_banks * 0x2000);
        offset += cart_info.no_of_ch_rom_banks * 0x2000;
    }

    if (cart_info.no_of_ch_rom_banks == 0)
    {
        cart->chr_ram = malloc(0x2000);
        memset(cart->chr_ram, 0, 0x2000);
        cart->ch_ram_size = 0x2000;
    }
}

int load_cartridge_from_memory(unsigned char *data, int len, Cartriadge *cart) {
    if (len < 16) {
        printf("ROM too small (%d bytes)\n", len);
        return -1;
    }
    // Read header first (16 bytes)
    unsigned char header[16];
    memcpy(header, data, 16);
    int offset = 16;

    iNesOneRomInfo rom_info = {};
    if(!get_rom_info(header, &rom_info)) return -1;

    cart->mirroring_mode = rom_info.flags6 & 1;
    cart->pg_rom_size = 0x4000 * rom_info.no_of_pg_rom_banks;
    // Extract mapper number
    int mapperId = (rom_info.flags7 & 0xF0) | (rom_info.flags6 >> 4);

    // Check for trainer (we'll skip it if present)
    int hasTrainer = rom_info.flags6 & 0x04;
    if (hasTrainer)
    {
        printf("Trainer section detected on catriadge.\n");
        offset += 512; // Skip trainer
    }

    if (offset + rom_info.no_of_pg_rom_banks * 0x4000 + rom_info.no_of_ch_rom_banks * 0x2000 > len)
    {
        printf("ROM data too small for declared sizes\n");
        return -3;
    }
    common_catridge_setup(cart, rom_info, data, offset);

    // Set mapper based on ID
    if (mapperId == 0)
    {
        mount_mapper_001_to_cartridge(cart, rom_info);
    }
    // else if (mapperId == 1)
    // {
    //     cart->mapper = M001;
    //     cart->ppu_read = M001_PPU;
    //     cart->cart_writer = M001_Write;
    //     cart->ppu_write = M001_PPU_WRITE;
    //     cart->pg_rom_bank_count = pgRomSize;
    //     cart->ch_rom_bank_count = chrRomSize;
    //     cart->pg_rom_bank_size = 0x4000; 
    //     cart->ch_rom_bank_size = chrRomSize == 0 ? 0 : 0x2000;
    //     cart->prg_ram = malloc(0x2000);
    //     memset(cart->prg_ram, 0, 0x2000);
    //     cart->prg_ram_size = 0x2000;
    // }
    // else if (mapperId == 2)
    // {
    //     cart->mapper = M002;
    //     cart->ppu_read = M002_PPU;
    //     cart->cart_writer = M002_Write;
    //     cart->ppu_write = M002_PPU_WRITE;
    //     cart->pg_rom_bank_count = pgRomSize;
    //     cart->ch_rom_bank_count = -1;
    //     cart->pg_rom_bank_size = 0x4000;
    //     cart->ch_rom_bank_size = 0x2000;
    // }
    // else if (mapperId == 3)
    // {
    //     cart->mapper = M003;
    //     cart->ppu_read = M003_PPU;
    //     cart->cart_writer = M003_Write;
    //     cart->ppu_write = M003_PPU_WRITE;
    //     cart->pg_rom_bank_count = pgRomSize;
    //     cart->pg_rom_bank_size = 0x4000;
    //     cart->ch_rom_bank_count = chrRomSize;
    //     cart->ch_rom_bank_size = 0x2000;
    // }
    // else if (mapperId == 4)
    // {
    //     cart->mapper = M004;
    //     cart->ppu_read = M004_PPU;
    //     cart->cart_writer = M004_Write;
    //     cart->ppu_write = M004_PPU_WRITE;
    //     cart->scanline_tick = M004_ScanlineTick;
    //     cart->pg_rom_bank_count = pgRomSize * 2;
    //     cart->pg_rom_bank_size = 0x2000;
    //     cart->ch_rom_bank_count = chrRomSize;
    //     cart->ch_rom_bank_size = 0x2000;
    //     cart->prg_ram = malloc(0x2000);
    //     memset(cart->prg_ram, 0, 0x2000);
    //     cart->prg_ram_size = 0x2000;
    // }
    // else
    // {
    //     printf("Warning: Unsupported mapper %d, defaulting to NROM (000)\n", mapperId);
    //     cart->mapper = M000;
    //     cart->ppu_read = M000_PPU;
    //     cart->cart_writer = NO_WRITE;
    // }
    printf("Successfully loaded cartridge from memory\n");
    printf("PRG-ROM: %dKB\n", rom_info.no_of_pg_rom_banks * 0x4000);
    printf("CHR-ROM: %dKB\n", rom_info.no_of_ch_rom_banks * 0x2000);
    printf("Mapper: %d\n", mapperId);

    connect_cartridge_to_bus(cart);
    return 0;
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

#include "rom_loader.h"
#include "bus.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define HT_IMPLEMENTATION
#include "ht.h"

static Ht(size_t, mount_mapper_to_catridge) mappers;

static inline void free_cart_and_close_file_ptr_after_error(Cartriadge *cart, FILE *fptr, const char *error_msg) {
  free(cart->pg_rom);
  free(cart->ch_rom);
  fclose(fptr);
  return;
}

void register_mapper(mount_mapper_to_catridge mapper, size_t index) {
  *ht_put(&mappers, index) = mapper;
}

void load_cartridge(char *filePath, Cartriadge *cart) {
  FILE *fptr = fopen(filePath, "rb");
  if (fptr == NULL) {
    printf("Could not load cartridge\n");
    return;
  }

  fseek(fptr, 0, SEEK_END);
  int rom_size = ftell(fptr);
  fseek(fptr, 0, SEEK_SET);
  unsigned char *file_contents = malloc(rom_size);
  fread(file_contents, rom_size, sizeof(unsigned char), fptr);
  load_cartridge_from_memory(file_contents, rom_size, cart);
  fclose(fptr);
}

static inline bool get_rom_info(const char *header, iNesOneRomInfo *info) {
  // Verify NES header magic number ("NES" followed by MS-DOS EOF)
  if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {
    printf("Invalid NES ROM format\n");
    return false;
  }

  info->no_of_pg_rom_banks = header[4];
  info->no_of_ch_rom_banks = header[5];
  info->flags6             = header[6];
  info->flags7             = header[7];

  /* iNES 2.0 detection: bits 2-3 of flags7 == 0x08 */
  if ((header[7] & 0x0C) == 0x08) {
    int submapper = header[8] & 0x0F;
    printf("iNES 2.0 ROM — submapper: %d\n", submapper);
  }

  return true;
}

void common_catridge_setup(Cartriadge *cart, iNesOneRomInfo cart_info, const char *data, int offset) {
  cart->scanline_tick = NULL;
  cart->prg_ram_size  = 0;
  cart->ch_ram_size   = 0;
  cart->cart_writer   = NULL;
  cart->chr_ram       = NULL;
  cart->prg_ram       = NULL;
  cart->ch_rom        = malloc(cart_info.no_of_ch_rom_banks * 0x2000);
  cart->pg_rom        = malloc(cart_info.no_of_pg_rom_banks * 0x4000);
  cart->size          = cart_info.no_of_pg_rom_banks * 0x4000 + cart_info.no_of_ch_rom_banks * 0x2000;

  memcpy(cart->pg_rom, data + offset, cart_info.no_of_pg_rom_banks * 0x4000);
  offset += cart_info.no_of_pg_rom_banks * 0x4000;

  if (cart_info.no_of_ch_rom_banks > 0) {
    memcpy(cart->ch_rom, data + offset, cart_info.no_of_ch_rom_banks * 0x2000);
    offset += cart_info.no_of_ch_rom_banks * 0x2000;
  }

  if (cart_info.no_of_ch_rom_banks == 0) {
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
  if (!get_rom_info(header, &rom_info))
    return -1;

  cart->mirroring_mode = rom_info.flags6 & 1;
  cart->pg_rom_size    = 0x4000 * rom_info.no_of_pg_rom_banks;
  // Extract mapper number
  int mapperId = (rom_info.flags7 & 0xF0) | (rom_info.flags6 >> 4);

  // Check for trainer (we'll skip it if present)
  int hasTrainer = rom_info.flags6 & 0x04;
  if (hasTrainer) {
    printf("Trainer section detected on catriadge.\n");
    offset += 512; // Skip trainer
  }

  if (offset + rom_info.no_of_pg_rom_banks * 0x4000 + rom_info.no_of_ch_rom_banks * 0x2000 > len) {
    printf("ROM data too small for declared sizes\n");
    return -3;
  }
  common_catridge_setup(cart, rom_info, data, offset);

  mount_mapper_to_catridge *cart_mapper = ht_find(&mappers, mapperId);

  if (cart_mapper == NULL) {
    printf("FATAL ERROR: Unsupported mapper %d, defaulting to NROM (000)\n", mapperId);
    return -2;
  } else
    (*cart_mapper)(cart, rom_info);

  printf("Successfully loaded cartridge from memory\n");
  printf("PRG-ROM: %dKB\n", rom_info.no_of_pg_rom_banks * 0x4000);
  printf("CHR-ROM: %dKB\n", rom_info.no_of_ch_rom_banks * 0x2000);
  printf("Mapper: %d\n", mapperId);

  connect_cartridge_to_bus(cart);
  return 0;
}

void load_cartridge_and_connect_to_bus(char *contents, int lenContents) {
  Cartriadge *test_cartridge = malloc(sizeof(Cartriadge));
  FILE       *f              = fopen("/tmp/cart.bin", "wb");
  fwrite(contents, 1, lenContents, f);
  fclose(f);
  load_cartridge("/tmp/cart.bin", test_cartridge);
  connect_cartridge_to_bus(test_cartridge);
  remove("/tmp/cart.bin");
}

static int isFlagSet(int position, unsigned char byte) {
  return byte & (1 << position);
}

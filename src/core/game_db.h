#ifndef GAME_DB_H
#define GAME_DB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define GAME_DB_BOARD_LEN 24

typedef struct {
    uint32_t crc32;
    uint8_t  mapper;
    char     board_type[GAME_DB_BOARD_LEN];
    uint32_t prg_rom_size;     /* bytes */
    uint32_t chr_size;         /* bytes, 0 = CHR RAM */
    uint32_t prg_ram_size;     /* bytes, 0 = no PRG RAM */
    bool     has_battery;
} GameDbEntry;

/* Parse the Nestopia XML database and build lookup table.
   Returns 0 on success. Call once at startup. */
int load_game_db(const char *xml_path);

/* Look up a game by PRG-ROM CRC32. Returns NULL if not found. */
const GameDbEntry *find_game(uint32_t crc32);

/* Free internal lookup table. Call at shutdown if needed. */
void free_game_db(void);

#endif

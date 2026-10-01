#ifndef GAME_DB_H
#define GAME_DB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define GAME_DB_BOARD_LEN 24

typedef struct {
    uint32_t crc32;
    char     board_type[GAME_DB_BOARD_LEN];
    uint32_t prg_rom_size;     /* bytes */
    uint32_t chr_size;         /* bytes, 0 = none (vram or chr) */
    bool     chr_is_ram;       /* true = <vram>, false = <chr> ROM */
    uint32_t prg_ram_size;     /* bytes, 0 = no PRG RAM */
    bool     has_battery;
} GameDbEntry;

/* Look up a game by PRG-ROM CRC32. Returns NULL if not found. */
const GameDbEntry *find_game(uint32_t crc32);

#endif

#include "game_db.h"
#include "db_data.h"
#include <stdlib.h>

static int attr_cmp(const void *a, const void *b) {
    const GameDbEntry *ea = (const GameDbEntry *)a;
    const GameDbEntry *eb = (const GameDbEntry *)b;
    if (ea->crc32 < eb->crc32) return -1;
    if (ea->crc32 > eb->crc32) return 1;
    return 0;
}

const GameDbEntry *find_game(uint32_t crc32) {
    GameDbEntry key = { .crc32 = crc32 };
    return (const GameDbEntry *)bsearch(&key, game_db_entries, game_db_count,
                                        sizeof(GameDbEntry), attr_cmp);
}

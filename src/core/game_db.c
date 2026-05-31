#include "game_db.h"
#include "db_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static GameDbEntry *g_entries = NULL;
static size_t       g_count   = 0;

/* ---- helpers ---- */

static uint32_t parse_crc(const char *s) {
    uint32_t v = 0;
    for (int i = 0; i < 8 && s[i]; i++) {
        v <<= 4;
        char c = s[i];
        if (c >= '0' && c <= '9')      v |= c - '0';
        else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
    }
    return v;
}

static uint32_t parse_size(const char *s) {
    uint32_t v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    if (*s == 'k' || *s == 'K') return v * 1024;
    if (*s == 'M' || *s == 'm') return v * 1024 * 1024;
    return v;
}

static const char *find_attr_bounded(const char *tag, const char *name, const char *end) {
    size_t name_len = strlen(name);
    const char *p = tag;
    while (p < end && (p = strstr(p, name)) != NULL && p < end) {
        if (p + name_len < end && p[name_len] == '=') {
            char q = p[name_len + 1];
            if (q == '"' || q == '\'') return p + name_len + 2;
        }
        p++;
    }
    return NULL;
}

static const char *find_tag_end(const char *p, const char *max) {
    const char *e = memchr(p, '>', max - p);
    return e ? e + 1 : max;
}

static int attr_cmp(const void *a, const void *b) {
    const GameDbEntry *ea = (const GameDbEntry *)a;
    const GameDbEntry *eb = (const GameDbEntry *)b;
    if (ea->crc32 < eb->crc32) return -1;
    if (ea->crc32 > eb->crc32) return 1;
    return 0;
}

/* ---- parser ---- */

int load_game_db(void) {
    char *buf = (char *)malloc(game_db_data_len + 1);
    if (!buf) return -1;
    memcpy(buf, game_db_data, game_db_data_len);
    buf[game_db_data_len] = '\0';

    size_t capacity = 1024;
    g_entries = (GameDbEntry *)malloc(capacity * sizeof(GameDbEntry));
    g_count   = 0;

    const char *p = buf;
    while ((p = strstr(p, "<game>")) != NULL) {
        p += 6;

        /* find <cartridge> or </game> */
        const char *cart = strstr(p, "<cartridge");
        if (!cart || cart >= strstr(p, "</game>")) continue;

        const char *cart_end = strstr(cart, ">");
        if (!cart_end) continue;
        cart_end++;

        /* extract CRC */
        const char *crc_str = find_attr_bounded(cart, "crc", cart_end);
        if (!crc_str) continue;

        /* find <board> */
        const char *board = strstr(cart_end, "<board");
        const char *game_end = strstr(p, "</game>");
        if (!board || !game_end || board >= game_end) continue;

        const char *board_tag_end = find_tag_end(board, game_end);
        if (!board_tag_end) continue;

        /* populate entry */
        GameDbEntry entry = {0};
        entry.crc32 = parse_crc(crc_str);

        /* mapper */
        const char *mat = find_attr_bounded(board, "mapper", board_tag_end);
        if (mat) entry.mapper = (uint8_t)atoi(mat);

        /* board type */
        const char *bt = find_attr_bounded(board, "type", board_tag_end);
        if (bt) {
            size_t n = 0;
            while (n < GAME_DB_BOARD_LEN - 1 && bt[n] && bt[n] != '"' && bt[n] != '\'')
                { entry.board_type[n] = bt[n]; n++; }
            entry.board_type[n] = '\0';
        }

        /* scan child elements within <board>...</board> */
        const char *child = board_tag_end;
        const char *board_end_tag = strstr(board, "</board>");
        const char *board_end = board_end_tag ? board_end_tag : game_end;

        while (child < board_end) {
            child = strstr(child, "<");
            if (!child || child >= board_end) break;

            const char *child_tag_end = find_tag_end(child, board_end);

            if (strncmp(child, "<prg ", 5) == 0) {
                const char *sz = find_attr_bounded(child, "size", child_tag_end);
                if (sz) entry.prg_rom_size = parse_size(sz);
            } else if (strncmp(child, "<chr ", 5) == 0) {
                const char *sz = find_attr_bounded(child, "size", child_tag_end);
                if (sz) { entry.chr_size = parse_size(sz); entry.chr_is_ram = false; }
            } else if (strncmp(child, "<vram ", 6) == 0) {
                const char *sz = find_attr_bounded(child, "size", child_tag_end);
                if (sz) { entry.chr_size = parse_size(sz); entry.chr_is_ram = true; }
            } else if (strncmp(child, "<wram ", 6) == 0) {
                const char *sz = find_attr_bounded(child, "size", child_tag_end);
                if (sz) entry.prg_ram_size = parse_size(sz);
                const char *bat = find_attr_bounded(child, "battery", child_tag_end);
                if (bat && *bat == '1') entry.has_battery = true;
            }

            child++;
        }

        if (entry.prg_rom_size == 0) continue; /* skip malformed */

        /* grow if needed */
        if (g_count >= capacity) {
            capacity *= 2;
            g_entries = (GameDbEntry *)realloc(g_entries, capacity * sizeof(GameDbEntry));
        }
        g_entries[g_count++] = entry;
    }

    free(buf);

    /* sort by CRC32 for binary search */
    qsort(g_entries, g_count, sizeof(GameDbEntry), attr_cmp);

    printf("Loaded %zu games from database\n", g_count);
    return 0;
}

const GameDbEntry *find_game(uint32_t crc32) {
    GameDbEntry key = { .crc32 = crc32 };
    return (const GameDbEntry *)bsearch(&key, g_entries, g_count,
                                        sizeof(GameDbEntry), attr_cmp);
}

void free_game_db(void) {
    free(g_entries);
    g_entries = NULL;
    g_count   = 0;
}

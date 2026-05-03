#ifndef PPU_H
#define PPU_H
#include "frameData.h"

unsigned char read_ppu(int addr);
void write_ppu(int addr, unsigned char byte);
void boot_ppu();
void kill_ppu();
FrameData *request_frame();
void draw_nametable_dbg();
void draw_tile_indices_dbg();
void render_pattern_table_debug();
void render_game_tile_indices(int offset_x);
void render_pattern_table_via_mapper(int offset_x);
void render_unified_debug(void);

#endif
#include "dev.h"
#include <string.h>
#include "core_bindings.h"

#define TILE_SIZE    8
#define NT_W         32
#define NT_H         30

static SDL_Window   *nt_window   = NULL;
static SDL_Renderer *nt_renderer = NULL;
static SDL_Texture  *nt_tex      = NULL;
static bool          nt_active   = false;
static bool          was_on      = false;

static void create_window(void) {
    int w = NT_W * TILE_SIZE * 2;
    int h = NT_H * TILE_SIZE * 2;
    nt_window   = SDL_CreateWindow("Nametable Viewer", w, h, SDL_WINDOW_RESIZABLE);
    nt_renderer = SDL_CreateRenderer(nt_window, "software");
    nt_tex = SDL_CreateTexture(nt_renderer, SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING, w, h);
    SDL_SetTextureScaleMode(nt_tex, SDL_SCALEMODE_NEAREST);
    register_debug_window(nt_window, "Nametable Viewer");
    nt_active = true;
}

static void destroy_window(void) {
    nt_active = false;
    unregister_debug_window(nt_window);
    if (nt_tex)      SDL_DestroyTexture(nt_tex);
    if (nt_renderer) SDL_DestroyRenderer(nt_renderer);
    if (nt_window)   SDL_DestroyWindow(nt_window);
    nt_tex = NULL; nt_renderer = NULL; nt_window = NULL;
}

static void render_tile(unsigned char *pixels, int pitch, int tile_x, int tile_y,
                        int tile_index, int palette_num) {
    int addr = tile_index * 16;
    const NesColor *sp = get_system_palette();
    for (int row = 0; row < 8; row++) {
        unsigned char low  = read_byte_ppu(addr + row);
        unsigned char high = read_byte_ppu(addr + 8 + row);
        for (int col = 0; col < 8; col++) {
            int bit = 7 - col;
            int color = ((low >> bit) & 1) | (((high >> bit) & 1) << 1);
            int pe = palette_num >= 0 ? read_palette_ram(palette_num * 4 + color) : color;
            NesColor c = sp[pe % 64];
            int px = ((tile_y * 8 + row) * pitch + (tile_x * 8 + col)) * 4;
            unsigned char *p = pixels + px;
            p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
        }
    }
}

static void init(void) {}
static void update(void) {}

static void render(void) {
    if (!rom_loaded) return;

    if (!was_on) {
        was_on = true;
        create_window();
    }

    if (!nt_active) return;
    if (!nt_window || !nt_renderer || !nt_tex) return;

    int w = NT_W * TILE_SIZE * 2;
    int h = NT_H * TILE_SIZE * 2;

    unsigned char *pixels = (unsigned char *)malloc(w * h * 4);
    int nt_offsets[] = {0x2000, 0x2400, 0x2800, 0x2C00};
    for (int nt = 0; nt < 4; nt++) {
        int ox = (nt % 2) * NT_W;
        int oy = (nt / 2) * NT_H;
        for (int row = 0; row < NT_H; row++) {
            for (int col = 0; col < NT_W; col++) {
                int tile_index = read_ppu_vram(nt_offsets[nt] + row * NT_W + col);
                int ar = row / 4, ac = col / 4;
                unsigned char attr = read_ppu_vram(nt_offsets[nt] + 0x3C0 + ar * 8 + ac);
                int sr = (row % 4) / 2, sc = (col % 4) / 2;
                int palette = (attr >> ((sr * 2 + sc) * 2)) & 0x03;
                render_tile(pixels, w, col + ox, row + oy, tile_index, palette);
            }
        }
    }

    SDL_UpdateTexture(nt_tex, NULL, pixels, w * 4);
    free(pixels);

    SDL_SetRenderDrawColor(nt_renderer, 0, 0, 0, 255);
    SDL_RenderClear(nt_renderer);
    int ww, wh;
    SDL_GetWindowSize(nt_window, &ww, &wh);
    float s = (float)ww / (float)w;
    if ((float)wh / (float)h < s) s = (float)wh / (float)h;
    SDL_FRect dst = {((float)ww - w * s) * 0.5f, ((float)wh - h * s) * 0.5f, w * s, h * s};
    SDL_RenderTexture(nt_renderer, nt_tex, NULL, &dst);
    SDL_RenderPresent(nt_renderer);
}

static void shutdown(void) {
    if (nt_active) destroy_window();
    was_on = false;
}

static DebugModule _mod = {
    "Nametable Viewer", init, update, render, shutdown, true, false
};
REGISTER_DEBUG_MODULE(_mod)

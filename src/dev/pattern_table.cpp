#include "dev.h"
#include <string.h>
#include "core_bindings.h"

#define TILE_SIZE 8
#define GRID      16

static SDL_Window   *pt_window   = NULL;
static SDL_Renderer *pt_renderer = NULL;
static SDL_Texture  *pt_tex      = NULL;
static bool          pt_active   = false;
static bool          was_on      = false;

static void create_window(void) {
    int w = GRID * TILE_SIZE * 2;
    int h = GRID * TILE_SIZE + 24;
    pt_window   = SDL_CreateWindow("Pattern Tables", w, h, SDL_WINDOW_RESIZABLE);
    pt_renderer = SDL_CreateRenderer(pt_window, "software");
    pt_tex = SDL_CreateTexture(pt_renderer, SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING, w, h);
    SDL_SetTextureScaleMode(pt_tex, SDL_SCALEMODE_NEAREST);
    register_debug_window(pt_window, "Pattern Tables");
    pt_active = true;
}

static void destroy_window(void) {
    pt_active = false;
    unregister_debug_window(pt_window);
    if (pt_tex)      SDL_DestroyTexture(pt_tex);
    if (pt_renderer) SDL_DestroyRenderer(pt_renderer);
    if (pt_window)   SDL_DestroyWindow(pt_window);
    pt_tex = NULL; pt_renderer = NULL; pt_window = NULL;
}

static void render_table(unsigned char *pixels, int pitch, int table_base, int offset_x) {
    const NesColor *sp = get_system_palette();
    for (int tile = 0; tile < 256; tile++) {
        int tx = (tile % GRID) * TILE_SIZE + offset_x;
        int ty = (tile / GRID) * TILE_SIZE;
        int addr = table_base + tile * 16;
        for (int row = 0; row < TILE_SIZE; row++) {
            unsigned char low  = read_byte_ppu(addr + row);
            unsigned char high = read_byte_ppu(addr + TILE_SIZE + row);
            for (int col = 0; col < TILE_SIZE; col++) {
                int bit = 7 - col;
                int color = ((low >> bit) & 1) | (((high >> bit) & 1) << 1);
                NesColor c = sp[color % 64];
                int px = ((ty + row) * pitch + (tx + col)) * 4;
                unsigned char *p = pixels + px;
                p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
            }
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

    if (!pt_active) return;
    if (!pt_window || !pt_renderer || !pt_tex) return;

    int w = GRID * TILE_SIZE * 2;
    int h = GRID * TILE_SIZE + 24;

    unsigned char *pixels = (unsigned char *)calloc(w * h, 4);
    render_table(pixels, w, 0x0000, 0);
    render_table(pixels, w, 0x1000, w / 2);

    const NesColor *sp = get_system_palette();
    int pal_y = GRID * TILE_SIZE + 4;
    for (int i = 0; i < 32; i++) {
        NesColor c = sp[read_palette_ram(i) % 64];
        int sx = i * 16;
        for (int r = 1; r < 15; r++) {
            for (int col = 1; col < 15; col++) {
                int px = ((pal_y + r) * w + (sx + col)) * 4;
                unsigned char *p = pixels + px;
                p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = 255;
            }
        }
    }
    SDL_UpdateTexture(pt_tex, NULL, pixels, w * 4);
    free(pixels);

    SDL_SetRenderDrawColor(pt_renderer, 0, 0, 0, 255);
    SDL_RenderClear(pt_renderer);
    int ww, wh;
    SDL_GetWindowSize(pt_window, &ww, &wh);
    float s = (float)ww / (float)w;
    if ((float)wh / (float)h < s) s = (float)wh / (float)h;
    SDL_FRect dst = {((float)ww - w * s) * 0.5f, ((float)wh - h * s) * 0.5f, w * s, h * s};
    SDL_RenderTexture(pt_renderer, pt_tex, NULL, &dst);
    SDL_RenderPresent(pt_renderer);
}

static void shutdown(void) {
    if (pt_active) destroy_window();
    was_on = false;
}

static DebugModule _mod = {
    "Pattern Tables", init, update, render, shutdown, true, false
};
REGISTER_DEBUG_MODULE(_mod)

#ifndef DEV_H
#define DEV_H
#include <SDL3/SDL.h>
#include <stdbool.h>

typedef struct DebugModule {
    const char *name;
    void (*init)(void);
    void (*update)(void);
    void (*render)(void);
    void (*shutdown)(void);
    bool requires_rom;
    bool enabled;
} DebugModule;

void register_debug_module(DebugModule *mod);

#define _REG_DBG_MODULE_2(mod, line)                                          \
    __attribute__((constructor)) static void _register_dbg_##line(void) {     \
        register_debug_module(&(mod));                                        \
    }
#define _REG_DBG_MODULE_1(mod, line) _REG_DBG_MODULE_2(mod, line)
#define REGISTER_DEBUG_MODULE(mod)    _REG_DBG_MODULE_1(mod, __LINE__)

void debug_init(SDL_Renderer *r);
void debug_menu(void);
void debug_update(void);
void debug_render(void);
void debug_shutdown(void);

void debug_handle_close(SDL_WindowID id);
void register_debug_window(SDL_Window *w, const char *module_name);
void unregister_debug_window(SDL_Window *w);
void set_game_texture(SDL_Texture *tex);
SDL_Texture *get_game_texture(void);
float get_game_fps(void);
int  get_bp_addr(void);
int  get_cycle_target(void);
bool get_cycle_break_on(void);
int  get_scaling_mode(void);
int  get_max_fps(void);
int  debug_module_count(void);

extern bool         rom_loaded;
extern bool         debug_paused;
extern SDL_Renderer *dev_renderer;
extern SDL_Window   *dev_window;

void start_rom(const char *path);

#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_littlefs.h"
#include "../../src/core/cartriadge.h"
#include "../../src/core/ControllerKeyStates.h"
#include "../../src/core/ppu.h"
#include "../../src/core/cpu.h"
#include "../../src/core/rom_loader.h"
#include "../../src/core/controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void app_main(void) {
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/games",
        .partition_label = "games",
        .format_if_mount_failed = false,
    };
    esp_vfs_littlefs_register(&conf);
    printf("HEAP SIZE REM: %ld\n", esp_get_free_heap_size());

    boot_ppu(5, 5, 5, 1);

    Cartriadge *cart = calloc(1, sizeof(Cartriadge));
    load_cartridge("/games/tetris2.nes", cart);
    connect_controller_to_console();

    boot_cpu();

    ControllerKeyStates keys = {0};
    for (;;) {
        FrameData *f = tick_cpu(&keys);
        if (f) {
            const uint16_t *px = (const uint16_t *)f->data;
            int             base = (240 / 2) * 256 + (256 / 2) - 2;
            for (int i = 0; i < 5; i++) {
                uint16_t v = px[base + i];
                printf("px[%d]=0x%04X r=%u g=%u b=%u a=%u\n",
                       base + i, v,
                       (unsigned)(v & 0x1F), (unsigned)((v >> 5) & 0x1F),
                       (unsigned)((v >> 10) & 0x1F), (unsigned)((v >> 15) & 1));
            }
        }
        vTaskDelay(1);
    }
}

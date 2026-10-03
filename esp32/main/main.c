#include "freertos/FreeRTOS.h"
#include "../../src/core/NesEmCore.h"
#include <stdio.h>
#include "esp_littlefs.h"



void app_main(void) {
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/games",
        .partition_label = "games",
        .format_if_mount_failed = false,
    };
    esp_vfs_littlefs_register(&conf);
    // FILE* file= fopen("/games/hello.txt", "rb");
    // char buffer[1024];
    // fgets(buffer, sizeof(buffer), file);
    printf("HEAP SIZE REM: %ld\n", esp_get_free_heap_size());

    boot_cpu();
}

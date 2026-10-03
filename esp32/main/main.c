#include "freertos/FreeRTOS.h"
#include "../../src/core/NesEmCore.h"
#include <stdio.h>

void app_main(void) {
    printf("Hellow world\n");
    boot_cpu();
}

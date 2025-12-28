#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "cartriadge.h"

int main() {
    printf("Starting emulator\n");
    Cartriadge* testCartriadge = malloc(sizeof(Cartriadge));
    loadCartriadge("test-roms/06-abs_xy.nes", testCartriadge);
    connectCartriadgeToBus(testCartriadge);
    bootCPU(true);
    runCPU();
    printf("Hello from my emulator\n");
    free(testCartriadge->mem);
    shutdownCPU();
    free(testCartriadge);
}
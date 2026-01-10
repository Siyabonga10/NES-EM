#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "cartriadge.h"
#include "ppu.h"

int main() {
    printf("Starting emulator\n");
    Cartriadge* testCartriadge = malloc(sizeof(Cartriadge));
    loadCartriadge("test-roms/05-absolute.nes", testCartriadge);
    connectCartriadgeToBus(testCartriadge);
    bootPPU();
    bootCPU(true);
    runCPU();
    printf("Hello from my emulator\n");
    free(testCartriadge->mem);
    shutdownCPU();
    killPPU();
    free(testCartriadge);
}
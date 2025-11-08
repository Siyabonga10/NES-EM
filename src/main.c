#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "cartriadge.h"

int main() {
    Cartriadge* testCartriadge;
    loadCartriadge("test-roms/01-implied.nes", testCartriadge);
    connectCartriadgeToBus(testCartriadge);
    bootCPU(true);
    runCPU();
    printf("Hello from my emulator\n");
    free(testCartriadge->mem);
    shutdownCPU();
}
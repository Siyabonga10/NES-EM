#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "cartriadge.h"
#include "ppu.h"

int main()
{
    Cartriadge *testCartriadge = malloc(sizeof(Cartriadge));
    loadCartriadge("test-roms/Dk.nes", testCartriadge);
    connectCartriadgeToBus(testCartriadge);
    bootPPU();
    bootCPU();
    runCPU();
    free(testCartriadge->mem);
    shutdownCPU();
    free(testCartriadge);
}
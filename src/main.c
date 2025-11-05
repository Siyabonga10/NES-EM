#include <stdio.h>
#include "cpu.h"

int main() {
    bootCPU();
    printf("Hello from my emulator\n");
    shutdownCPU();
}
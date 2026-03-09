#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "cartriadge.h"
#include "ppu.h"

static char *test_files[] = {
    "test-roms/01-implied.nes",
    "test-roms/02-immediate.nes",
    "test-roms/03-zero_page.nes",
    "test-roms/04-zp_xy.nes",
    "test-roms/05-absolute.nes",
    "test-roms/06-abs_xy.nes",
    "test-roms/07-ind_x.nes",
    "test-roms/08-ind_y.nes",
    "test-roms/09-branches.nes",
    "test-roms/10-stack.nes",
    "test-roms/11-jmp_jsr.nes",
    "test-roms/12-rts.nes",
    "test-roms/13-rti.nes",
    "test-roms/14-brk.nes",
    "test-roms/15-special.nes"};

int main()
{
    Cartriadge *testCartriadge = malloc(sizeof(Cartriadge));
    loadCartriadge("test-roms\\tetris.nes", testCartriadge);
    connectCartriadgeToBus(testCartriadge);
    bootPPU();
    bootCPU();
    runCPU();
    free(testCartriadge->mem);
    shutdownCPU();
    free(testCartriadge);
}
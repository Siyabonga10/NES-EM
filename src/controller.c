#include "controller.h"
#include "bus.h"

unsigned char readController(int addr)
{
    return 0xFF;
}

void writeController(int addr, unsigned char value)
{
}

void connectControllerToConsole()
{
    connectController(readController, writeController);
}
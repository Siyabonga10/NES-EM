#include "controller.h"
#include "bus.h"
#include "controllerKeys.h"
#include <raylib.h>
#include <stdio.h>

#define CONTROLLERS_REG_SIZE 1
#define MAX_REG_INDEX 8

static unsigned char registers[CONTROLLERS_REG_SIZE] = {0};
static unsigned int reg_index = 0;

static unsigned char snapshot = {0};
static unsigned char current_state = {0};

unsigned char readController(int addr)
{
    if (reg_index >= MAX_REG_INDEX)
        return 0x01;
    return (snapshot >> (reg_index++)) & 0x01;
}

void writeController(int addr, unsigned char value)
{
    if (addr != 0x4016)
        return;
    registers[0] = value;
    unsigned char strobe = registers[0] & 1;
    if (strobe == 0)
    {
        snapshot = current_state;
        reg_index = 0;
    }
}

void connectControllerToConsole()
{
    connectController(readController, writeController);
}

void updateControllerInput()
{
    current_state = 0;
    if (IsKeyDown(KEY_A))
        current_state |= (1 << A);
    if (IsKeyDown(KEY_B))
        current_state |= (1 << B);
    if (IsKeyDown(KEY_ENTER))
        current_state |= (1 << SELECT);
    if (IsKeyDown(KEY_SPACE))
        current_state |= (1 << START);

    if (IsKeyDown(KEY_UP))
        current_state |= (1 << UP);
    if (IsKeyDown(KEY_DOWN))
        current_state |= (1 << DOWN);
    if (IsKeyDown(KEY_LEFT))
        current_state |= (1 << LEFT);
    if (IsKeyDown(KEY_RIGHT))
        current_state |= (1 << RIGHT);
}
#include "controller.h"
#include "bus.h"
#include "controllerKeys.h"
#include "ControllerKeyStates.h"
#include <stdio.h>

#define CONTROLLERS_REG_SIZE 1
#define MAX_REG_INDEX 8

static unsigned char registers[CONTROLLERS_REG_SIZE] = {0};
static unsigned int reg_index = 0;

static unsigned char snapshot = {0};
static unsigned char current_state = {0};
static unsigned char snapshot2 = {0};
static unsigned int reg_index2 = 0;

static unsigned char strobe_state = 0;

unsigned char readController(int addr)
{
    if (addr == 0x4017)
    {
        if (strobe_state)
            return 0;
        if (reg_index2 >= MAX_REG_INDEX)
            return 0x01;
        return (snapshot2 >> (reg_index2++)) & 0x01;
    }
    if (strobe_state)
        return current_state & 0x01;
    if (reg_index >= MAX_REG_INDEX)
        return 0x01;
    unsigned char bit = (snapshot >> (reg_index++)) & 0x01;
    return bit;
}

void writeController(int addr, unsigned char value)
{
    if (addr != 0x4016)
        return;
    strobe_state = value & 1;
    if (strobe_state == 0)
    {
        snapshot = current_state;
        reg_index = 0;
        snapshot2 = 0;
        reg_index2 = 0;
    }
}
void connect_controller_to_console()
{
    connect_controller(readController, writeController);
}

void update_controller_input(ControllerKeyStates *keyStates)
{
    current_state = 0;
    if (keyStates->a_pressed)
        current_state |= (1 << A);
    if (keyStates->b_pressed)
        current_state |= (1 << B);
    if (keyStates->select_pressed)
        current_state |= (1 << SELECT);
    if (keyStates->start_pressed)
        current_state |= (1 << START);

    if (keyStates->up_pressed)
        current_state |= (1 << UP);
    if (keyStates->down_pressed)
        current_state |= (1 << DOWN);
    if (keyStates->left_pressed)
        current_state |= (1 << LEFT);
    if (keyStates->right_pressed)
        current_state |= (1 << RIGHT);
}
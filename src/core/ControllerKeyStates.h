#ifndef CONTROLLER_KEY_STATES
#define CONTROLLER_KEY_STATES
#include <stdbool.h>
typedef struct
{
    bool a_pressed;
    bool b_pressed;
    bool up_pressed;
    bool down_pressed;
    bool left_pressed;
    bool right_pressed;
    bool start_pressed;
    bool select_pressed;
} ControllerKeyStates;

#endif
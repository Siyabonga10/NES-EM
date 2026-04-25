#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "ControllerKeyStates.h"

void connect_controller_to_console();
void update_controller_input(ControllerKeyStates *keyStates);

#endif
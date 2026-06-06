#ifndef CONTROLLER_SAVE_STATE_H
#define CONTROLLER_SAVE_STATE_H
#include <stdint.h>

typedef struct {
  unsigned char registers;
  unsigned int  reg_index;
  unsigned char snapshot;
  unsigned char strobe_state;
  unsigned char snapshot2;
  unsigned int  reg_index2;
} ControllerSaveState;

#endif

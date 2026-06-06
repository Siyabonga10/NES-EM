#ifndef REGISTER_SAVE_STATE_H
#define REGISTER_SAVE_STATE_H
#include "../bus.h"

#ifdef _MSC_VER
#pragma section(".CRT$XCZ", read)
#define REGISTER_SAVE_STATE(label, saver, loader)           \
  static void register_save_state_##saver(void) {           \
    register_savable_component(label, saver, loader);       \
  }                                                         \
  __declspec(allocate(".CRT$XCZ")) void (*_reg_##saver)(void) = register_save_state_##saver;
#else
#define REGISTER_SAVE_STATE(label, saver, loader)                                \
  __attribute__((constructor)) static void register_save_state_##saver(void) {   \
    register_savable_component(label, saver, loader);                            \
  }
#endif

#endif

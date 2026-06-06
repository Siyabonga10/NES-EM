#ifndef SAVE_STATE_INFO_H
#define SAVE_STATE_INFO_H
#include <stdint.h>
#define SECTION_LABEL_SIZE 10

typedef struct Save_State_Info {
  char          section_label[SECTION_LABEL_SIZE];
  uint32_t      content_length;
  unsigned char *content;
} Save_State_Info;

typedef void (*save_section_state)(Save_State_Info *save_buffer, uint32_t allowable_content_length);
typedef void (*load_section)(Save_State_Info *section_data);

#endif
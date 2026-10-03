#ifndef FRAME_DATA_H
#define FRAME_DATA_H
#include <stdbool.h>
#include <stdlib.h>


typedef struct {
  volatile bool is_new_frame;
  size_t        width;
  size_t        height;
  void         *data;
} FrameData;

#endif
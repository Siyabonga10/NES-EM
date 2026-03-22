#ifndef FRAME_DATA_H
#define FRAME_DATA_H
#include <stdbool.h>
#include <stdlib.h>
#include "nesColor.h"

typedef struct {
    bool is_new_frame;
    size_t width;
    size_t height;
    NesColor* data;
} FrameData;

#endif
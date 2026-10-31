#ifndef DRAWING_H
#define DRAWING_H

#include "common.h"

typedef struct {
    int width;
    int height;
    int pitch;
    uint32_t *pixels;
} OffscreenBuffer;

void draw_gradient(OffscreenBuffer *buffer, int x_offset, int y_offset);
void draw_random_pixels(OffscreenBuffer *buffer);

#endif /* DRAWING_H */

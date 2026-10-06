#include "drawing.h"
#include <stdlib.h>

#if RAND_MAX == 32767
#define Rand32() ((rand() << 16) + (rand() << 1) + (rand() & 1))
#else
#define Rand32() rand()
#endif

void draw_gradient(OffscreenBuffer *buffer, int x_offset, int y_offset)
{
    uint8_t *row;
    uint32_t *pixel;
    int x, y;
    uint8_t blue, green, red;

    if (!buffer || !buffer->pixels) 
    {
        return;
    }

    row = (uint8_t *)buffer->pixels;
    red = 0;

    for (y = 0; y < buffer->height; ++y) {
        pixel = (uint32_t *)row;
        for (x = 0; x < buffer->width; ++x) {
            blue  = (uint8_t)(x + x_offset);
            green = (uint8_t)(y + y_offset);

            *pixel++ = (red << 16) | (green << 8) | blue;
        }

        row += buffer->pitch;
    }
}

void draw_random_pixels(OffscreenBuffer *buffer)
{
    static unsigned int index = 0;
    int total_pixels;

    if (!buffer || !buffer->pixels || buffer->width <= 0 || buffer->height <= 0)
    {
        return;
    }

    total_pixels = buffer->width * buffer->height;

    buffer->pixels[(index++) % total_pixels] = (uint32_t)Rand32();
    buffer->pixels[Rand32() % total_pixels]  = 0x00000000;
}

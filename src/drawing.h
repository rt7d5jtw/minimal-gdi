#ifndef DRAWING_H
#define DRAWING_H

// Integer Types (handle missing <stdint.h> for Visual C++ 2008 and older)
#if defined(_MSC_VER) && (_MSC_VER <= 1500)
    typedef signed __int8      int8_t;
    typedef signed __int16     int16_t;
    typedef signed __int32     int32_t;
    typedef signed __int64     int64_t;
    typedef unsigned __int8    uint8_t;
    typedef unsigned __int16   uint16_t;
    typedef unsigned __int32   uint32_t;
    typedef unsigned __int64   uint64_t;
#  if defined(_WIN64)
    typedef signed __int64     intptr_t;
    typedef unsigned __int64   uintptr_t;
#  else
    typedef signed __int32     intptr_t;
    typedef unsigned __int32   uintptr_t;
#  endif
#else
#  include <stdint.h>
#endif

typedef struct {
    int width;
    int height;
    int pitch;
    uint32_t *pixels;
} OffscreenBuffer;

void draw_gradient(OffscreenBuffer *buffer, int x_offset, int y_offset);
void draw_random_pixels(OffscreenBuffer *buffer);

#endif // DRAWING_H

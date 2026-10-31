#ifndef COMMON_H
#define COMMON_H

#define global static
#define internal static

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

#ifndef UNICODE
#  define UNICODE
#endif

#if !defined(__cplusplus)
#  if defined(_MSC_VER) && (_MSC_VER >= 1800)
#    include <stdbool.h>
#  elif defined(_MSC_VER)
#    ifndef __bool_true_false_are_defined
       typedef unsigned char bool;
#      define true  1
#      define false 0
#      define __bool_true_false_are_defined 1
#    endif
#  elif (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__GNUC__) || defined(__clang__)
#    include <stdbool.h>
#  else
#    ifndef __bool_true_false_are_defined
       typedef unsigned char bool;
#      define true  1
#      define false 0
#      define __bool_true_false_are_defined 1
#    endif
#  endif
#endif

/* Integer Types */
#  if defined(_MSC_VER) && (_MSC_VER <= 1500)
typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#    if defined(_WIN64)
typedef signed __int64 intptr_t;
typedef unsigned __int64 uintptr_t;
#    else
typedef signed __int32 intptr_t;
typedef unsigned __int32 uintptr_t;
#    endif
#  else
#    include <stdint.h>
#  endif

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

typedef s8  b8;
typedef s16 b16;
typedef s32 b32;
typedef s64 b64;

#endif /* COMMON_H */

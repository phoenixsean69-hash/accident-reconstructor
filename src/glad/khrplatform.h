#ifndef __khrplatform_h_
#define __khrplatform_h_

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

/* Khronos platform types used by the generated OpenGL declarations. */
typedef int8_t    khronos_int8_t;
typedef uint8_t   khronos_uint8_t;
typedef int16_t   khronos_int16_t;
typedef uint16_t  khronos_uint16_t;
typedef int32_t   khronos_int32_t;
typedef uint32_t  khronos_uint32_t;
typedef intptr_t  khronos_intptr_t;
typedef ptrdiff_t khronos_ssize_t;

typedef khronos_int8_t   GLbyte;
typedef khronos_uint8_t  GLubyte;
typedef khronos_int16_t  GLshort;
typedef khronos_uint16_t GLushort;
typedef khronos_int32_t  GLint;
typedef khronos_uint32_t GLuint;
typedef khronos_int32_t  GLclampx;
typedef khronos_intptr_t GLintptr;
typedef khronos_ssize_t  GLsizeiptr;

#endif

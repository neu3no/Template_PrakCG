#ifndef __glext_h

#define __glext_h
#include <GL/glut.h>
#include "glext.h"

PFNGLGENBUFFERSPROC glGenBuffers = NULL;
PFNGLBINDBUFFERPROC glBindBuffer = NULL;
PFNGLBUFFERDATAPROC glBufferData = NULL;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
#endif

int init_extensions(void);
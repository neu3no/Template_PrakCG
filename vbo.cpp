// Dieses File sowie die zugehoerige header wird von make unter linux bzw. Mac
// OSX ignoriert

#include "vbo.h"

int init_extensions(void) {
	glGenBuffers = (PFNGLGENBUFFERSARBPROC) wglGetProcAddress("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERARBPROC) wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAARBPROC) wglGetProcAddress("glBufferData");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) wglGetProcAddress("glDeleteBuffers");

	if (glGenBuffers) return (1);
	else return (0);
}
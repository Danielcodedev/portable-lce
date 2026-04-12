#include "platform/renderer/IRenderPath.h"

#include "java/FloatBuffer.h"
#include "java/IntBuffer.h"

// GL compat function stubs for game code that calls gl* functions
// via the compatibility macros in gl_compat.h. These provide the
// non-template overloads that get linked against.

void glGetFloat(int, FloatBuffer*) {}

int glGenTextures_4J() { return 0; }
void glGenTextures_4J(int, unsigned int*) {}
void glDeleteTextures_4J(int) {}
void glDeleteTextures_4J(int, const unsigned int*) {}
void glTexImage2D_4J(int, int, int, int, int, int, int, int, void*) {}

void glCallLists_4J(IntBuffer*) {}

void glLight_4J(int, int, FloatBuffer*) {}
void glLightModel_4J(int, FloatBuffer*) {}
void glFog_4J(int, FloatBuffer*) {}
void glTexGen_4J(int, int, FloatBuffer*) {}

void glReadPixels_4J(int, int, int, int, int, int, void*) {}
void glReadPixels_4J(int, int, int, int, int, int, unsigned char*) {}

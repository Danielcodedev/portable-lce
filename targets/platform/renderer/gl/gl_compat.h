#pragma once

// Compatibility shim that routes legacy gl* calls through IRenderPath.
// This file defines GL_* constants and gl* macros so that existing game
// code compiles without modification. No actual GL headers are included.

#include <cstdint>
#include <cstdlib>

#include "platform/renderer/IRenderPath.h"

#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_TEXTURE
#define GL_TEXTURE 0x1702
#endif
#ifndef GL_S
#define GL_S 0x2000
#endif
#ifndef GL_T
#define GL_T 0x2001
#endif
#ifndef GL_R
#define GL_R 0x2002
#endif
#ifndef GL_Q
#define GL_Q 0x2003
#endif
#ifndef GL_TEXTURE_GEN_S
#define GL_TEXTURE_GEN_S 0x0C60
#endif
#ifndef GL_TEXTURE_GEN_T
#define GL_TEXTURE_GEN_T 0x0C61
#endif
#ifndef GL_TEXTURE_GEN_Q
#define GL_TEXTURE_GEN_Q 0x0C63
#endif
#ifndef GL_TEXTURE_GEN_R
#define GL_TEXTURE_GEN_R 0x0C62
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_CULL_FACE
#define GL_CULL_FACE 0x0B44
#endif
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif
#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_LIGHTING
#define GL_LIGHTING 0x0B50
#endif
#ifndef GL_LIGHT0
#define GL_LIGHT0 0x4000
#endif
#ifndef GL_LIGHT1
#define GL_LIGHT1 0x4001
#endif
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#endif
#ifndef GL_ONE_MINUS_SRC_ALPHA
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#endif
#ifndef GL_ONE
#define GL_ONE 1
#endif
#ifndef GL_ZERO
#define GL_ZERO 0
#endif
#ifndef GL_DST_COLOR
#define GL_DST_COLOR 0x0306
#endif
#ifndef GL_ONE_MINUS_DST_COLOR
#define GL_ONE_MINUS_DST_COLOR 0x0307
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_CONSTANT_ALPHA
#define GL_CONSTANT_ALPHA 0x8003
#endif
#ifndef GL_ONE_MINUS_CONSTANT_ALPHA
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#endif
#ifndef GL_GREATER
#define GL_GREATER 0x0204
#endif
#ifndef GL_EQUAL
#define GL_EQUAL 0x0202
#endif
#ifndef GL_LEQUAL
#define GL_LEQUAL 0x0203
#endif
#ifndef GL_ALWAYS
#define GL_ALWAYS 0x0207
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif
#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_EXP
#define GL_EXP 0x0800
#endif
#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif
#ifndef GL_FOG_START
#define GL_FOG_START 0x0B63
#endif
#ifndef GL_FOG_END
#define GL_FOG_END 0x0B64
#endif
#ifndef GL_FOG_MODE
#define GL_FOG_MODE 0x0B65
#endif
#ifndef GL_FOG_DENSITY
#define GL_FOG_DENSITY 0x0B62
#endif
#ifndef GL_FOG_COLOR
#define GL_FOG_COLOR 0x0B66
#endif
#ifndef GL_POSITION
#define GL_POSITION 0x1203
#endif
#ifndef GL_AMBIENT
#define GL_AMBIENT 0x1200
#endif
#ifndef GL_DIFFUSE
#define GL_DIFFUSE 0x1201
#endif
#ifndef GL_SPECULAR
#define GL_SPECULAR 0x1202
#endif
#ifndef GL_LIGHT_MODEL_AMBIENT
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003
#endif
#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 0x803A
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_COMPILE
#define GL_COMPILE 0x1300
#endif
#ifndef GL_SMOOTH
#define GL_SMOOTH 0x1D01
#endif
#ifndef GL_FLAT
#define GL_FLAT 0x1D00
#endif
#ifndef GL_FRONT
#define GL_FRONT 0x0404
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif
#ifndef GL_AMBIENT_AND_DIFFUSE
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#endif
#ifndef GL_COLOR_MATERIAL
#define GL_COLOR_MATERIAL 0x0B57
#endif
#ifndef GL_NORMALIZE
#define GL_NORMALIZE 0x0BA1
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif

// Viewport type constants (match the old renderer enum values)
#define VIEWPORT_TYPE_FULLSCREEN 0
#define VIEWPORT_TYPE_SPLIT_TOP 1
#define VIEWPORT_TYPE_SPLIT_BOTTOM 2
#define VIEWPORT_TYPE_SPLIT_LEFT 3
#define VIEWPORT_TYPE_SPLIT_RIGHT 4
#define VIEWPORT_TYPE_QUADRANT_TOP_LEFT 5
#define VIEWPORT_TYPE_QUADRANT_TOP_RIGHT 6
#define VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT 7
#define VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT 8

#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
#ifndef GL_DST_ALPHA
#define GL_DST_ALPHA 0x0304
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_GEQUAL
#define GL_GEQUAL 0x0206
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_POLYGON_OFFSET_FILL
#define GL_POLYGON_OFFSET_FILL 0x8037
#endif
#ifndef GL_POLYGON_OFFSET_LINE
#define GL_POLYGON_OFFSET_LINE 0x2A02
#endif
#ifndef GL_TEXTURE_GEN_MODE
#define GL_TEXTURE_GEN_MODE 0x2500
#endif
#ifndef GL_OBJECT_LINEAR
#define GL_OBJECT_LINEAR 0x2401
#endif
#ifndef GL_EYE_LINEAR
#define GL_EYE_LINEAR 0x2400
#endif
#ifndef GL_OBJECT_PLANE
#define GL_OBJECT_PLANE 0x2501
#endif
#ifndef GL_EYE_PLANE
#define GL_EYE_PLANE 0x2502
#endif

// Type compat
typedef int eViewportType;

// Display list macros
#define glNewList(_list, _mode) RenderPath.CBuffStart(_list)
#define glEndList() RenderPath.CBuffEnd()
#define glCallList(_list) ((void)RenderPath.CBuffCall(_list))
#define glGenLists(range) RenderPath.CBuffCreate(range)
#define glDeleteLists(list, range) RenderPath.CBuffDelete(list, range)

inline void glShadeModel(int) {}
inline void glTexGeni(int, int, int) {}
#define glTranslatef(x, y, z) do { RenderPath.MatrixTranslate(x, y, z); } while(0)
#define glRotatef(a, x, y, z) do { RenderPath.MatrixRotate((a)*(3.14159265358979f/180.f), x, y, z); } while(0)
#define glScalef(x, y, z) do { RenderPath.MatrixScale(x, y, z); } while(0)
#define glScaled(x, y, z) do { RenderPath.MatrixScale((float)(x),(float)(y),(float)(z)); } while(0)
#define glPushMatrix() do { RenderPath.MatrixPush(); } while(0)
#define glPopMatrix() do { RenderPath.MatrixPop(); } while(0)
#define glLoadIdentity() do { RenderPath.MatrixSetIdentity(); } while(0)
#define glMatrixMode(mode) do { RenderPath.MatrixMode(mode); } while(0)
#define glMultMatrixf(m) do { RenderPath.MatrixMult(m); } while(0)
#define glColor4f(r, g, b, a) do { RenderPath.StateSetColour(r, g, b, a); } while(0)
#define glColor3f(r, g, b) do { RenderPath.StateSetColour(r, g, b, 1.0f); } while(0)
#define glAlphaFunc(func, ref) do { RenderPath.StateSetAlphaFunc(func, ref); } while(0)

#define glEnable(cap) do {                                              \
    if ((cap)==0x0B60) RenderPath.StateSetFogEnable(true);              \
    else if ((cap)==0x0B50) RenderPath.StateSetLightingEnable(true);    \
    else if ((cap)==0x0BC0) RenderPath.StateSetAlphaTestEnable(true);   \
    else if ((cap)==0x0DE1) RenderPath.StateSetTextureEnable(true);     \
    else if ((cap)==0x0BE2) RenderPath.StateSetBlendEnable(true);       \
    else if ((cap)==0x0B44) RenderPath.StateSetFaceCull(true);          \
    else if ((cap)==0x0B71) RenderPath.StateSetDepthTestEnable(true);   \
    else if ((cap)==0x4000) RenderPath.StateSetLightEnable(0, true);    \
    else if ((cap)==0x4001) RenderPath.StateSetLightEnable(1, true);    \
} while(0)

#define glDisable(cap) do {                                             \
    if ((cap)==0x0B60) RenderPath.StateSetFogEnable(false);             \
    else if ((cap)==0x0B50) RenderPath.StateSetLightingEnable(false);   \
    else if ((cap)==0x0BC0) RenderPath.StateSetAlphaTestEnable(false);  \
    else if ((cap)==0x0DE1) RenderPath.StateSetTextureEnable(false);    \
    else if ((cap)==0x0BE2) RenderPath.StateSetBlendEnable(false);      \
    else if ((cap)==0x0B44) RenderPath.StateSetFaceCull(false);         \
    else if ((cap)==0x0B71) RenderPath.StateSetDepthTestEnable(false);  \
    else if ((cap)==0x4000) RenderPath.StateSetLightEnable(0, false);   \
    else if ((cap)==0x4001) RenderPath.StateSetLightEnable(1, false);   \
} while(0)

#define glFogi(pname, param) do { if ((pname)==0x0B65) RenderPath.StateSetFogMode(param); } while(0)
#define glFogf(pname, param) do {                                           \
    if ((pname)==0x0B63) RenderPath.StateSetFogNearDistance(param);          \
    else if ((pname)==0x0B64) RenderPath.StateSetFogFarDistance(param);      \
    else if ((pname)==0x0B62) RenderPath.StateSetFogDensity(param);          \
} while(0)

#define glOrtho(l, r, b, t, n, f) do { RenderPath.MatrixOrthogonal(l, r, b, t, n, f); } while(0)
#define glMultiTexCoord2f(tex, u, v) do { if ((tex)==0x84C1) RenderPath.StateSetVertexTextureUV(u, v); } while(0)
#define glActiveTexture(tex) do { RenderPath.StateSetActiveTexture(tex); } while(0)
#define glClientActiveTexture(tex) do { RenderPath.StateSetActiveTexture(tex); } while(0)
#define glBlendFunc(s, d) do { RenderPath.StateSetBlendFunc(s, d); } while(0)
#define glDepthMask(e) do { RenderPath.StateSetDepthMask(e); } while(0)
#define glClear(f) do { RenderPath.Clear(f); } while(0)
#define glClearColor(r, g, b, a) do { float cc[4]={r,g,b,a}; RenderPath.SetClearColour(cc); } while(0)
#define glViewport(x, y, w, h) do {} while(0)
#define glFlush() do {} while(0)
#define glNormal3f(x, y, z) do {} while(0)
#define glColorMaterial(face, mode) do {} while(0)
#define glLineWidth(w) do { RenderPath.StateSetLineWidth(w); } while(0)
#define glClearDepth(d) do {} while(0)
#define glDepthFunc(f) do { RenderPath.StateSetDepthFunc(f); } while(0)
#define glCullFace(mode) do {} while(0)
#define glPixelStorei(pname, param) do {} while(0)
#define glPolygonOffset(factor, units) do { RenderPath.StateSetDepthSlopeAndBias(factor, units); } while(0)
#define glBindTexture(target, id) do { RenderPath.TextureBind(id); } while(0)
#define glTexParameteri(target, pname, param) do { RenderPath.TextureSetParam(pname, param); } while(0)

// Function stubs declared in bgfx/gl_stubs.cpp
class FloatBuffer;
class IntBuffer;
void glGetFloat(int pname, FloatBuffer* params);
int glGenTextures_4J();
void glGenTextures_4J(int n, unsigned int* textures);
void glDeleteTextures_4J(int id);
void glDeleteTextures_4J(int n, const unsigned int* textures);
void glTexImage2D_4J(int, int, int, int, int, int, int, int, void*);
void glCallLists_4J(IntBuffer* lists);
void glLight_4J(int light, int pname, FloatBuffer* params);
void glLightModel_4J(int pname, FloatBuffer* params);
void glFog_4J(int pname, FloatBuffer* params);
void glTexGen_4J(int coord, int pname, FloatBuffer* params);
void glReadPixels_4J(int x, int y, int w, int h, int format, int type, void* pixels);
void glReadPixels_4J(int x, int y, int w, int h, int format, int type, unsigned char* pixels);

template <typename T> inline void glGenTextures_4J(T* buf) { buf->put(0); buf->flip(); }
template <typename T> inline void glDeleteTextures_4J(T*) {}
template <typename T> inline void glTexCoordPointer_4J(int, int, T*) {}
template <typename T> inline void glNormalPointer_4J(int, T*) {}
template <typename T> inline void glColorPointer_4J(int, bool, int, T*) {}
template <typename T> inline void glVertexPointer_4J(int, int, T*) {}
template <typename T> inline void glTexImage2D_4J(int,int,int,int,int,int,int,int,T*) {}
template <typename T> inline void glReadPixels_4J(int,int,int,int,int,int,T*) {}

#define glGenTextures(...) glGenTextures_4J(__VA_ARGS__)
#define glDeleteTextures(...) glDeleteTextures_4J(__VA_ARGS__)
#define glTexCoordPointer(a, b, c) glTexCoordPointer_4J(a, b, c)
#define glNormalPointer(a, b) glNormalPointer_4J(a, b)
#define glColorPointer(a, b, c, d) glColorPointer_4J(a, b, c, d)
#define glVertexPointer(a, b, c) glVertexPointer_4J(a, b, c)
#define glTexImage2D(a,b,c,d,e,f,g,h,i) glTexImage2D_4J(a,b,c,d,e,f,g,h,i)
#define glCallLists(x) glCallLists_4J(x)
#define glReadPixels(a,b,c,d,e,f,g) glReadPixels_4J(a,b,c,d,e,f,g)
#define glFog(a, b) glFog_4J(a, b)
#define glLight(a, b, c) glLight_4J(a, b, c)
#define glLightModel(a, b) glLightModel_4J(a, b)
#define glTexGen(a, b, c) glTexGen_4J(a, b, c)

#include "platform/renderer/IRenderPath.h"
#include "platform/renderer/renderer.h"

#include "java/FloatBuffer.h"
#include "java/IntBuffer.h"

namespace platform_internal {
IPlatformRenderer& PlatformRenderer_get() {
    static struct StubRenderer : IPlatformRenderer {
        void Initialise() override {}
        void InitialiseContext() override {}
        void Tick() override {}
        void StartFrame() override {}
        void Present() override {}
        void Clear(int) override {}
        void SetClearColour(const float[4]) override {}
        void Shutdown() override {}
        void Suspend() override {}
        bool Suspended() override { return false; }
        void Resume() override {}
        void SetWindowSize(int, int) override {}
        void SetFullscreen(bool) override {}
        bool IsWidescreen() override { return false; }
        bool IsHiDef() override { return false; }
        void GetFramebufferSize(int& w, int& h) override { w = 0; h = 0; }
        bool ShouldClose() override { return false; }
        void Close() override {}
        void UpdateGamma(unsigned short) override {}
        void MatrixMode(int) override {}
        void MatrixSetIdentity() override {}
        void MatrixTranslate(float, float, float) override {}
        void MatrixRotate(float, float, float, float) override {}
        void MatrixScale(float, float, float) override {}
        void MatrixPerspective(float, float, float, float) override {}
        void MatrixOrthogonal(float, float, float, float, float, float) override {}
        void MatrixPop() override {}
        void MatrixPush() override {}
        void MatrixMult(float*) override {}
        const float* MatrixGet(int) override { static float id[16]={}; return id; }
        void Set_matrixDirty() override {}
        void DrawVertices(ePrimitiveType, int, void*, eVertexType, ePixelShaderType) override {}
        void CBuffLockStaticCreations() override {}
        int CBuffCreate(int) override { return -1; }
        void CBuffDelete(int, int) override {}
        void CBuffDeleteAll() override {}
        void CBuffStart(int, bool) override {}
        void CBuffClear(int) override {}
        int CBuffSize(int) override { return 0; }
        void CBuffEnd() override {}
        bool CBuffCall(int, bool) override { return false; }
        void CBuffTick() override {}
        void CBuffDeferredModeStart() override {}
        void CBuffDeferredModeEnd() override {}
        int TextureCreate() override { return -1; }
        void TextureFree(int) override {}
        void TextureBind(int) override {}
        void TextureBindVertex(int, bool) override {}
        void TextureSetTextureLevels(int) override {}
        int TextureGetTextureLevels() override { return 1; }
        void TextureData(int, int, void*, int, eTextureFormat) override {}
        void TextureDataUpdate(int, int, int, int, void*, int) override {}
        void TextureSetParam(int, int) override {}
        int LoadTextureData(const char*, D3DXIMAGE_INFO*, int**) override { return -1; }
        int LoadTextureData(uint8_t*, uint32_t, D3DXIMAGE_INFO*, int**) override { return -1; }
        void ReadPixels(int, int, int, int, void*) override {}
        void StateSetColour(float, float, float, float) override {}
        void StateSetDepthMask(bool) override {}
        void StateSetBlendEnable(bool) override {}
        void StateSetBlendFunc(int, int) override {}
        void StateSetBlendFactor(unsigned int) override {}
        void StateSetAlphaFunc(int, float) override {}
        void StateSetDepthFunc(int) override {}
        void StateSetFaceCull(bool) override {}
        void StateSetFaceCullCW(bool) override {}
        void StateSetLineWidth(float) override {}
        void StateSetWriteEnable(bool, bool, bool, bool) override {}
        void StateSetDepthTestEnable(bool) override {}
        void StateSetAlphaTestEnable(bool) override {}
        void StateSetDepthSlopeAndBias(float, float) override {}
        void StateSetFogEnable(bool) override {}
        void StateSetFogMode(int) override {}
        void StateSetFogNearDistance(float) override {}
        void StateSetFogFarDistance(float) override {}
        void StateSetFogDensity(float) override {}
        void StateSetFogColour(float, float, float) override {}
        void StateSetLightingEnable(bool) override {}
        void StateSetVertexTextureUV(float, float) override {}
        void StateSetLightColour(int, float, float, float) override {}
        void StateSetLightAmbientColour(float, float, float) override {}
        void StateSetLightDirection(int, float, float, float) override {}
        void StateSetLightEnable(int, bool) override {}
        void StateSetViewport(eViewportType) override {}
        void StateSetEnableViewportClipPlanes(bool) override {}
        void StateSetTexGenCol(int, float, float, float, float, bool) override {}
        void StateSetStencil(int, uint8_t, uint8_t, uint8_t) override {}
        void StateSetForceLOD(int) override {}
        void StateSetTextureEnable(bool) override {}
        void StateSetActiveTexture(int) override {}
        void SetChunkOffset(float, float, float) override {}
        void BeginConditionalSurvey(int) override {}
        void EndConditionalSurvey() override {}
        void BeginConditionalRendering(int) override {}
        void EndConditionalRendering() override {}
        void CaptureThumbnail(ImageFileBuffer*) override {}
        void CaptureScreen(ImageFileBuffer*, XSOCIAL_PREVIEWIMAGE*) override {}
        void BeginEvent(const char*) override {}
        void EndEvent() override {}
    } instance;
    return instance;
}
}

// GL compat function stubs for code that calls gl* functions directly
// (Camera.cpp, Lighting.cpp, MemoryTracker.cpp, etc.)
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

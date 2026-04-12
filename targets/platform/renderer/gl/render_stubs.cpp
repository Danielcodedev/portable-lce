#include "GLRenderer.h"

// Command Buffers
void GLRenderer::CBuffLockStaticCreations() {}
int GLRenderer::CBuffSize(int) { return 0; }
void GLRenderer::CBuffTick() {}
void GLRenderer::CBuffDeferredModeStart() {}
void GLRenderer::CBuffDeferredModeEnd() {}

// Render States
void GLRenderer::StateSetLightEnable(int, bool) {}
void GLRenderer::StateSetEnableViewportClipPlanes(bool) {}
void GLRenderer::StateSetForceLOD(int) {}
void GLRenderer::StateSetTexGenCol(int, float, float, float, float, bool) {}

// Screen/Image Capturing
void GLRenderer::CaptureThumbnail(ImageFileBuffer*) {}
void GLRenderer::CaptureScreen(ImageFileBuffer*, XSOCIAL_PREVIEWIMAGE*) {}

// Conditional Rendering & Events
void GLRenderer::BeginConditionalSurvey(int) {}
void GLRenderer::EndConditionalSurvey() {}
void GLRenderer::BeginConditionalRendering(int) {}
void GLRenderer::EndConditionalRendering() {}
void GLRenderer::BeginEvent(const char*) {}
void GLRenderer::EndEvent() {}
void GLRenderer::Tick() {}

// Lifecycle
void GLRenderer::Suspend() {}
bool GLRenderer::Suspended() { return false; }
void GLRenderer::Resume() {}

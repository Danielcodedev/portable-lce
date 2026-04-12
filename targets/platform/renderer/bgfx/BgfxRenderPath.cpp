#include "BgfxRenderPath.h"

#include <cstring>

#include <SDL.h>
#include <SDL_syswm.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace rp;

static constexpr uint32_t TRANSIENT_ARENA_SIZE = 16 * 1024 * 1024;

// Embedded minimal shaders (GLSL source for bgfx's GL backend).
// In production these would be compiled with shaderc; for bootstrap we
// use bgfx's built-in debug text program or the noop renderer.

BgfxRenderPath::BgfxRenderPath(SDL_Window* window) : window_(window) {
    transient_arena_.resize(TRANSIENT_ARENA_SIZE);
    projection_stack_.push(glm::mat4(1.0f));
    modelview_stack_.push(glm::mat4(1.0f));

    SDL_GetWindowSize(window_, (int*)&width_, (int*)&height_);

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    SDL_GetWindowWMInfo(window_, &wmi);

    bgfx::renderFrame(); // single-threaded mode signal

    bgfx::Init init;
#if defined(__linux__)
    init.platformData.ndt = wmi.info.x11.display;
    init.platformData.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif defined(_WIN32)
    init.platformData.nwh = wmi.info.win.window;
#endif
    init.type = bgfx::RendererType::OpenGL;
    init.resolution.width = width_;
    init.resolution.height = height_;
    init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx::init(init);

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, width_, height_);

    // Vertex layout matching world_standard (32 bytes)
    vl_world_standard_
        .begin()
        .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::Normal,    4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Int16, true)
        .end();

    fb_.width = width_;
    fb_.height = height_;
    fb_.aspect = (float)width_ / (float)height_;
    fb_.is_widescreen = fb_.aspect > 1.5f;
    fb_.is_hi_def = height_ >= 720;

    bgfx_state_ = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LEQUAL |
                   BGFX_STATE_CULL_CCW;
}

BgfxRenderPath::~BgfxRenderPath() {
    if (bgfx::isValid(program_))
        bgfx::destroy(program_);
    bgfx::shutdown();
}

// -- Matrix stack -----------------------------------------------------------

std::stack<glm::mat4>& BgfxRenderPath::current_stack() {
    return (matrix_mode_ == 0x1701) ? projection_stack_ : modelview_stack_;
}

void BgfxRenderPath::MatrixMode(int type)     { matrix_mode_ = type; }
void BgfxRenderPath::MatrixSetIdentity()      { current_stack().top() = glm::mat4(1.0f); }
void BgfxRenderPath::MatrixPush()             { current_stack().push(current_stack().top()); }
void BgfxRenderPath::MatrixPop()              { if (current_stack().size() > 1) current_stack().pop(); }

void BgfxRenderPath::MatrixTranslate(float x, float y, float z) {
    current_stack().top() = glm::translate(current_stack().top(), glm::vec3(x, y, z));
}

void BgfxRenderPath::MatrixRotate(float angle, float x, float y, float z) {
    current_stack().top() = glm::rotate(current_stack().top(), angle, glm::vec3(x, y, z));
}

void BgfxRenderPath::MatrixScale(float x, float y, float z) {
    current_stack().top() = glm::scale(current_stack().top(), glm::vec3(x, y, z));
}

void BgfxRenderPath::MatrixPerspective(float fovy, float aspect, float zNear, float zFar) {
    current_stack().top() = glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
}

void BgfxRenderPath::MatrixOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar) {
    current_stack().top() = glm::ortho(left, right, bottom, top, zNear, zFar);
}

void BgfxRenderPath::MatrixMult(float* m) {
    current_stack().top() *= glm::make_mat4(m);
}

const float* BgfxRenderPath::MatrixGet(int type) {
    if (type == 0x0BA7) return glm::value_ptr(projection_stack_.top());
    return glm::value_ptr(modelview_stack_.top());
}

// -- State accumulator ------------------------------------------------------

void BgfxRenderPath::StateSetColour(float r, float g, float b, float a) {
    tint_color_[0] = r; tint_color_[1] = g; tint_color_[2] = b; tint_color_[3] = a;
}

void BgfxRenderPath::StateSetDepthMask(bool e) {
    depth_write_ = e;
    bgfx_state_ = (bgfx_state_ & ~BGFX_STATE_WRITE_Z) | (e ? BGFX_STATE_WRITE_Z : 0);
}

void BgfxRenderPath::StateSetBlendEnable(bool e) { blend_enabled_ = e; }

void BgfxRenderPath::StateSetBlendFunc(int, int) {
    // Simplified: most common is src_alpha/one_minus_src_alpha
    bgfx_state_ = (bgfx_state_ & ~BGFX_STATE_BLEND_MASK) |
                   (blend_enabled_ ? BGFX_STATE_BLEND_ALPHA : 0);
}

void BgfxRenderPath::StateSetBlendFactor(unsigned int) {}
void BgfxRenderPath::StateSetAlphaFunc(int, float) {}

void BgfxRenderPath::StateSetDepthFunc(int) {
    bgfx_state_ = (bgfx_state_ & ~BGFX_STATE_DEPTH_TEST_MASK) |
                   (depth_test_enabled_ ? BGFX_STATE_DEPTH_TEST_LEQUAL : 0);
}

void BgfxRenderPath::StateSetFaceCull(bool e) {
    cull_enabled_ = e;
    bgfx_state_ = (bgfx_state_ & ~BGFX_STATE_CULL_MASK) |
                   (e ? BGFX_STATE_CULL_CCW : 0);
}

void BgfxRenderPath::StateSetLineWidth(float) {}

void BgfxRenderPath::StateSetWriteEnable(bool r, bool g, bool b, bool a) {
    bgfx_state_ &= ~(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    if (r || g || b) bgfx_state_ |= BGFX_STATE_WRITE_RGB;
    if (a) bgfx_state_ |= BGFX_STATE_WRITE_A;
}

void BgfxRenderPath::StateSetDepthTestEnable(bool e) {
    depth_test_enabled_ = e;
    bgfx_state_ = (bgfx_state_ & ~BGFX_STATE_DEPTH_TEST_MASK) |
                   (e ? BGFX_STATE_DEPTH_TEST_LEQUAL : 0);
}

void BgfxRenderPath::StateSetAlphaTestEnable(bool) {}

// Fog (stored but not applied - needs shader support)
void BgfxRenderPath::StateSetFogEnable(bool) {}
void BgfxRenderPath::StateSetFogMode(int) {}
void BgfxRenderPath::StateSetFogNearDistance(float) {}
void BgfxRenderPath::StateSetFogFarDistance(float) {}
void BgfxRenderPath::StateSetFogDensity(float) {}
void BgfxRenderPath::StateSetFogColour(float, float, float) {}

// Lighting (stored but not applied - needs shader support)
void BgfxRenderPath::StateSetLightingEnable(bool) {}
void BgfxRenderPath::StateSetLightColour(int, float, float, float) {}
void BgfxRenderPath::StateSetLightAmbientColour(float, float, float) {}
void BgfxRenderPath::StateSetLightDirection(int, float, float, float) {}
void BgfxRenderPath::StateSetLightEnable(int, bool) {}

void BgfxRenderPath::StateSetViewport(int) {}
void BgfxRenderPath::StateSetEnableViewportClipPlanes(bool) {}
void BgfxRenderPath::StateSetStencil(int, uint8_t, uint8_t, uint8_t) {}
void BgfxRenderPath::StateSetForceLOD(int) {}
void BgfxRenderPath::StateSetTextureEnable(bool e) { texture_enabled_ = e; }
void BgfxRenderPath::StateSetActiveTexture(int) {}

void BgfxRenderPath::SetChunkOffset(float x, float y, float z) {
    chunk_offset_[0] = x; chunk_offset_[1] = y; chunk_offset_[2] = z;
}

void BgfxRenderPath::StateSetVertexTextureUV(float, float) {}

// -- DrawVertices -----------------------------------------------------------

void BgfxRenderPath::DrawVertices(int, int count, void* data, int, int) {
    if (count <= 0 || !data) return;

    if (bgfx::getAvailTransientVertexBuffer(count, vl_world_standard_) < (uint32_t)count)
        return;
    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, count, vl_world_standard_);
    memcpy(tvb.data, data, count * vl_world_standard_.getStride());

    glm::mat4 mvp = projection_stack_.top() * modelview_stack_.top();
    bgfx::setTransform(glm::value_ptr(mvp));
    bgfx::setState(bgfx_state_ | (blend_enabled_ ? BGFX_STATE_BLEND_ALPHA : 0));
    bgfx::setVertexBuffer(0, &tvb);

    if (bgfx::isValid(program_))
        bgfx::submit(current_view_id_, program_);
    else
        bgfx::discard();
}

// -- Resource methods -------------------------------------------------------

MeshHandle BgfxRenderPath::create_mesh(const MeshDesc&)          { return kInvalidMesh; }
void BgfxRenderPath::update_mesh(MeshHandle, const MeshDesc&)    {}
void BgfxRenderPath::destroy_mesh(MeshHandle)                     {}

TextureHandle BgfxRenderPath::create_texture(const TextureDesc& desc) {
    auto mem = bgfx::copy(desc.initial_data.data(), desc.initial_data.size());
    auto th = bgfx::createTexture2D(desc.width, desc.height, false, 1,
                                     bgfx::TextureFormat::RGBA8, 0, mem);
    for (uint32_t i = 0; i < textures_.size(); ++i) {
        if (!textures_[i].occupied) {
            textures_[i] = {th, ++textures_[i].generation, desc.width, desc.height, true};
            return {i, textures_[i].generation};
        }
    }
    uint32_t idx = (uint32_t)textures_.size();
    textures_.push_back({th, 1, desc.width, desc.height, true});
    return {idx, 1};
}

void BgfxRenderPath::update_texture(TextureHandle, const TextureRegion&) {}

void BgfxRenderPath::destroy_texture(TextureHandle h) {
    if (h.index < textures_.size() && textures_[h.index].generation == h.generation) {
        bgfx::destroy(textures_[h.index].bgfx_handle);
        textures_[h.index].occupied = false;
    }
}

MaterialHandle BgfxRenderPath::create_material(const MaterialDesc& desc) {
    for (uint32_t i = 0; i < materials_.size(); ++i) {
        if (!materials_[i].occupied) {
            materials_[i] = {desc, ++materials_[i].generation, true};
            return {i, materials_[i].generation};
        }
    }
    uint32_t idx = (uint32_t)materials_.size();
    materials_.push_back({desc, 1, true});
    return {idx, 1};
}

void BgfxRenderPath::update_material(MaterialHandle h, const MaterialDesc& desc) {
    if (h.index < materials_.size() && materials_[h.index].generation == h.generation)
        materials_[h.index].desc = desc;
}

void BgfxRenderPath::destroy_material(MaterialHandle h) {
    if (h.index < materials_.size() && materials_[h.index].generation == h.generation)
        materials_[h.index].occupied = false;
}

std::pair<TransientVertexBuffer, std::span<std::byte>>
BgfxRenderPath::alloc_transient_vertices(uint32_t count, VertexLayout, PrimitiveType prim) {
    uint32_t stride = vl_world_standard_.getStride();
    uint32_t bytes = count * stride;
    if (transient_offset_ + bytes > transient_arena_.size())
        return {{}, {}};
    TransientVertexBuffer tvb;
    tvb.frame_index = current_frame_;
    tvb.offset = transient_offset_;
    tvb.vertex_count = count;
    tvb.primitive = prim;
    auto span = std::span<std::byte>(transient_arena_.data() + transient_offset_, bytes);
    transient_offset_ += bytes;
    return {tvb, span};
}

// -- Frame submission -------------------------------------------------------

void BgfxRenderPath::render_frame(const FrameDesc& frame) {
    transient_offset_ = 0;
    current_frame_++;
    fb_ = frame.framebuffer;

    bgfx::setViewRect(0, 0, 0, width_, height_);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::touch(0);
    bgfx::frame();
}

void BgfxRenderPath::resize(uint32_t w, uint32_t h) {
    width_ = w;
    height_ = h;
    bgfx::reset(w, h, BGFX_RESET_VSYNC);
}

const FrameFramebuffer& BgfxRenderPath::framebuffer() const { return fb_; }
void BgfxRenderPath::read_framebuffer(const TextureReadback&) {}
ResourceFootprint BgfxRenderPath::query_resource_footprint() const { return {}; }
void BgfxRenderPath::seal_static_resource_tier() {}
void BgfxRenderPath::begin_atomic_resource_batch() {}
void BgfxRenderPath::end_atomic_resource_batch() {}
void BgfxRenderPath::push_debug_event(const char*) {}
void BgfxRenderPath::pop_debug_event() {}
void BgfxRenderPath::tick() {}

// -- Command buffer stubs (no display list equivalent in bgfx) --------------

int  BgfxRenderPath::CBuffCreate(int) { return -1; }
void BgfxRenderPath::CBuffDelete(int, int) {}
void BgfxRenderPath::CBuffDeleteAll() {}
void BgfxRenderPath::CBuffStart(int, bool) {}
void BgfxRenderPath::CBuffClear(int) {}
int  BgfxRenderPath::CBuffSize(int) { return 0; }
void BgfxRenderPath::CBuffEnd() {}
bool BgfxRenderPath::CBuffCall(int, bool) { return false; }
void BgfxRenderPath::CBuffDeferredModeStart() {}
void BgfxRenderPath::CBuffDeferredModeEnd() {}

// -- Texture legacy stubs ---------------------------------------------------

int  BgfxRenderPath::TextureCreate() { return -1; }
void BgfxRenderPath::TextureFree(int) {}
void BgfxRenderPath::TextureBind(int idx) { bound_texture_ = idx; }
void BgfxRenderPath::TextureBindVertex(int, bool) {}
void BgfxRenderPath::TextureSetTextureLevels(int) {}
void BgfxRenderPath::TextureData(int, int, void*, int, int) {}
void BgfxRenderPath::TextureDataUpdate(int, int, int, int, void*, int) {}
void BgfxRenderPath::TextureSetParam(int, int) {}
int  BgfxRenderPath::TextureGetTextureLevels() { return 1; }
void BgfxRenderPath::ReadPixels(int, int, int, int, void*) {}
int  BgfxRenderPath::LoadTextureData(const char*, void*, int**) { return -1; }
int  BgfxRenderPath::LoadTextureData(uint8_t*, uint32_t, void*, int**) { return -1; }

// -- Frame lifecycle --------------------------------------------------------

void BgfxRenderPath::StartFrame() {}
void BgfxRenderPath::Present() {}
void BgfxRenderPath::Clear(int) {}
void BgfxRenderPath::SetClearColour(const float[4]) {}
void BgfxRenderPath::Set_matrixDirty() {}
void BgfxRenderPath::CBuffLockStaticCreations() {}

// -- Window -----------------------------------------------------------------

void BgfxRenderPath::GetFramebufferSize(int& w, int& h) { w = width_; h = height_; }
bool BgfxRenderPath::IsWidescreen() { return fb_.is_widescreen; }
bool BgfxRenderPath::IsHiDef() { return fb_.is_hi_def; }
void BgfxRenderPath::Close() { should_close_ = true; }
bool BgfxRenderPath::ShouldClose() { return should_close_; }
void BgfxRenderPath::SetWindowSize(int w, int h) { SDL_SetWindowSize(window_, w, h); resize(w, h); }
void BgfxRenderPath::SetFullscreen(bool fs) { SDL_SetWindowFullscreen(window_, fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0); }
void BgfxRenderPath::UpdateGamma(unsigned short) {}
void BgfxRenderPath::Suspend() {}
bool BgfxRenderPath::Suspended() { return false; }
void BgfxRenderPath::Resume() {}

void BgfxRenderPath::BeginEvent(const char*) {}
void BgfxRenderPath::EndEvent() {}

void BgfxRenderPath::submit_immediate(const DrawCall&) {}

// -- Factory ----------------------------------------------------------------

std::unique_ptr<rp::IRenderPath> make_bgfx_render_path(SDL_Window* window) {
    return std::make_unique<BgfxRenderPath>(window);
}

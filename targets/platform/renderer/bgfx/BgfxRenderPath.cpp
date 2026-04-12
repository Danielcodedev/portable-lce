#include "BgfxRenderPath.h"

#include <cstring>

#include <SDL.h>
#include <SDL_syswm.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "platform/PlatformTypes.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vs_4jcraft.bin.h"
#include "fs_4jcraft.bin.h"

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

    // Load shaders
    bgfx::ShaderHandle vsh = bgfx::createShader(
        bgfx::makeRef(vs_4jcraft_glsl, sizeof(vs_4jcraft_glsl)));
    bgfx::ShaderHandle fsh = bgfx::createShader(
        bgfx::makeRef(fs_4jcraft_glsl, sizeof(fs_4jcraft_glsl)));
    program_ = bgfx::createProgram(vsh, fsh, true);

    // Vertex uniforms
    u_baseColor_    = bgfx::createUniform("u_baseColor",    bgfx::UniformType::Vec4);
    u_chunkOffset_  = bgfx::createUniform("u_chunkOffset",  bgfx::UniformType::Vec4);
    u_lightParams_  = bgfx::createUniform("u_lightParams",  bgfx::UniformType::Vec4);
    u_light0Dir_    = bgfx::createUniform("u_light0Dir",    bgfx::UniformType::Vec4);
    u_light1Dir_    = bgfx::createUniform("u_light1Dir",    bgfx::UniformType::Vec4);
    u_lightDiffuse_ = bgfx::createUniform("u_lightDiffuse", bgfx::UniformType::Vec4);
    u_lightAmbient_ = bgfx::createUniform("u_lightAmbient", bgfx::UniformType::Vec4);
    u_fogParams_    = bgfx::createUniform("u_fogParams",    bgfx::UniformType::Vec4);
    u_lmTransform_  = bgfx::createUniform("u_lmTransform",  bgfx::UniformType::Vec4);
    u_globalLM_     = bgfx::createUniform("u_globalLM",     bgfx::UniformType::Vec4);
    // Fragment uniforms
    u_fragParams_   = bgfx::createUniform("u_fragParams",   bgfx::UniformType::Vec4);
    u_fogColor_     = bgfx::createUniform("u_fogColor",     bgfx::UniformType::Vec4);
    s_tex0_         = bgfx::createUniform("s_tex0",         bgfx::UniformType::Sampler);
    s_tex1_         = bgfx::createUniform("s_tex1",         bgfx::UniformType::Sampler);
}

BgfxRenderPath::~BgfxRenderPath() {
    if (bgfx::isValid(program_))
        bgfx::destroy(program_);
    bgfx::shutdown();
}

// -- Matrix stack -----------------------------------------------------------

std::stack<glm::mat4>& BgfxRenderPath::current_stack() {
    return (matrix_mode_ == rp::MatrixStack::projection) ? projection_stack_ : modelview_stack_;
}

void BgfxRenderPath::MatrixMode(rp::MatrixStack stack)     { matrix_mode_ = stack; }
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

const float* BgfxRenderPath::MatrixGet(rp::MatrixStack stack) {
    if (stack == rp::MatrixStack::projection) return glm::value_ptr(projection_stack_.top());
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

void BgfxRenderPath::StateSetBlendFunc(rp::BlendFactor, rp::BlendFactor) {
    // Simplified: most common is src_alpha/one_minus_src_alpha
    bgfx_state_ = (bgfx_state_ & ~BGFX_STATE_BLEND_MASK) |
                   (blend_enabled_ ? BGFX_STATE_BLEND_ALPHA : 0);
}

void BgfxRenderPath::StateSetBlendFactor(unsigned int) {}
void BgfxRenderPath::StateSetAlphaFunc(rp::AlphaTest, float r) { alpha_ref_ = r; }

void BgfxRenderPath::StateSetDepthFunc(rp::DepthTest) {
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
void BgfxRenderPath::StateSetDepthSlopeAndBias(float, float) {}

// Fog (stored but not applied - needs shader support)
void BgfxRenderPath::StateSetFogEnable(bool e) { fog_enabled_ = e; }
void BgfxRenderPath::StateSetFogMode(rp::FogMode m) { fog_mode_ = static_cast<float>(m); }
void BgfxRenderPath::StateSetFogNearDistance(float d) { fog_start_ = d; }
void BgfxRenderPath::StateSetFogFarDistance(float d) { fog_end_ = d; }
void BgfxRenderPath::StateSetFogDensity(float d) { fog_density_ = d; }
void BgfxRenderPath::StateSetFogColour(float r, float g, float b) { fog_color_[0]=r; fog_color_[1]=g; fog_color_[2]=b; fog_color_[3]=1; }

// Lighting (stored but not applied - needs shader support)
void BgfxRenderPath::StateSetLightingEnable(bool e) { lighting_enabled_ = e; }
void BgfxRenderPath::StateSetLightColour(int, float r, float g, float b) { light_diffuse_[0]=r; light_diffuse_[1]=g; light_diffuse_[2]=b; }
void BgfxRenderPath::StateSetLightAmbientColour(float r, float g, float b) { light_ambient_[0]=r; light_ambient_[1]=g; light_ambient_[2]=b; }
void BgfxRenderPath::StateSetLightDirection(int l, float x, float y, float z) { float* d = l==0 ? light0_dir_ : light1_dir_; d[0]=x; d[1]=y; d[2]=z; }
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

void BgfxRenderPath::DrawVertices(int primType, int count, void* data, int, int) {
    if (count <= 0 || !data) return;

    uint32_t stride = vl_world_standard_.getStride();

    // Convert triangle fan to triangle list (bgfx doesn't support fans)
    // Fan: v0,v1,v2,v3 -> Triangles: v0,v1,v2, v0,v2,v3
    int submitCount = count;
    bool isFan = (primType == 0x0006); // GL_TRIANGLE_FAN
    if (isFan && count >= 3) {
        submitCount = (count - 2) * 3;
    }

    if (bgfx::getAvailTransientVertexBuffer(submitCount, vl_world_standard_) < (uint32_t)submitCount)
        return;
    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, submitCount, vl_world_standard_);

    if (isFan && count >= 3) {
        const uint8_t* src = (const uint8_t*)data;
        uint8_t* dst = tvb.data;
        for (int i = 1; i < count - 1; i++) {
            memcpy(dst, src, stride); dst += stride;                    // v0
            memcpy(dst, src + i * stride, stride); dst += stride;      // vi
            memcpy(dst, src + (i+1) * stride, stride); dst += stride;  // vi+1
        }
    } else {
        memcpy(tvb.data, data, count * stride);
    }

    // Set primitive type in bgfx state
    uint64_t primState = 0;
    if (primType == 0x0005) primState = BGFX_STATE_PT_TRISTRIP;      // GL_TRIANGLE_STRIP
    else if (primType == 0x0001) primState = BGFX_STATE_PT_LINES;    // GL_LINES
    else if (primType == 0x0003) primState = BGFX_STATE_PT_LINESTRIP; // GL_LINE_STRIP

    glm::mat4 mvp = projection_stack_.top() * modelview_stack_.top();
    bgfx::setTransform(glm::value_ptr(mvp));
    bgfx::setState(bgfx_state_ | (blend_enabled_ ? BGFX_STATE_BLEND_ALPHA : 0) | primState);
    bgfx::setVertexBuffer(0, &tvb);

    // Vertex uniforms
    bgfx::setUniform(u_baseColor_, tint_color_);
    float chunkOff[4] = { chunk_offset_[0], chunk_offset_[1], chunk_offset_[2], 0 };
    bgfx::setUniform(u_chunkOffset_, chunkOff);
    float lp[4] = { lighting_enabled_ ? 1.0f : 0.0f, 1.0f, 0, 0 };
    bgfx::setUniform(u_lightParams_, lp);
    bgfx::setUniform(u_light0Dir_, light0_dir_);
    bgfx::setUniform(u_light1Dir_, light1_dir_);
    bgfx::setUniform(u_lightDiffuse_, light_diffuse_);
    bgfx::setUniform(u_lightAmbient_, light_ambient_);
    float fp[4] = { fog_enabled_ ? fog_mode_ : 0.0f, fog_start_, fog_end_, fog_density_ };
    bgfx::setUniform(u_fogParams_, fp);
    float lmt[4] = { 1, 1, 0, 0 };
    bgfx::setUniform(u_lmTransform_, lmt);
    bgfx::setUniform(u_globalLM_, global_lm_);
    // Bind texture if available
    bool hasTexture = false;
    if (texture_enabled_ && bound_texture_ >= 0) {
        auto it = gl_tex_to_bgfx_.find(bound_texture_);
        if (it != gl_tex_to_bgfx_.end()) {
            bgfx::setTexture(0, s_tex0_, it->second);
            hasTexture = true;
        }
    }
    // Fragment uniforms
    float fragP[4] = { hasTexture ? 1.0f : 0.0f, 0.0f, alpha_ref_, fog_enabled_ ? 1.0f : 0.0f };
    bgfx::setUniform(u_fragParams_, fragP);
    bgfx::setUniform(u_fogColor_, fog_color_);

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

int BgfxRenderPath::TextureCreate() {
    int id = next_gl_tex_id_++;
    return id;
}

void BgfxRenderPath::TextureFree(int idx) {
    auto it = gl_tex_to_bgfx_.find(idx);
    if (it != gl_tex_to_bgfx_.end()) {
        bgfx::destroy(it->second);
        gl_tex_to_bgfx_.erase(it);
    }
}

void BgfxRenderPath::TextureBind(int idx) {
    bound_texture_ = idx;
}

void BgfxRenderPath::TextureBindVertex(int, bool) {}
void BgfxRenderPath::TextureSetTextureLevels(int) {}

void BgfxRenderPath::TextureData(int width, int height, void* data, int, int) {
    if (bound_texture_ < 0 || !data || width <= 0 || height <= 0) return;
    auto it = gl_tex_to_bgfx_.find(bound_texture_);
    if (it != gl_tex_to_bgfx_.end()) {
        bgfx::destroy(it->second);
    }
    const bgfx::Memory* mem = bgfx::copy(data, width * height * 4);
    bgfx::TextureHandle th = bgfx::createTexture2D(
        width, height, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT, mem);
    gl_tex_to_bgfx_[bound_texture_] = th;
}

void BgfxRenderPath::TextureDataUpdate(int xoff, int yoff, int w, int h, void* data, int) {
    if (bound_texture_ < 0 || !data) return;
    auto it = gl_tex_to_bgfx_.find(bound_texture_);
    if (it == gl_tex_to_bgfx_.end()) return;
    const bgfx::Memory* mem = bgfx::copy(data, w * h * 4);
    bgfx::updateTexture2D(it->second, 0, 0, xoff, yoff, w, h, mem);
}

void BgfxRenderPath::TextureSetParam(int, int) {}
int  BgfxRenderPath::TextureGetTextureLevels() { return 1; }
void BgfxRenderPath::ReadPixels(int, int, int, int, void*) {}
int BgfxRenderPath::LoadTextureData(const char* filename, void* srcInfo, int** dataOut) {
    int w, h, channels;
    unsigned char* pixels = stbi_load(filename, &w, &h, &channels, 4);
    if (!pixels) return -1;
    auto* info = static_cast<D3DXIMAGE_INFO*>(srcInfo);
    if (info) { info->Width = w; info->Height = h; }
    int* rgba = new int[w * h];
    memcpy(rgba, pixels, w * h * 4);
    stbi_image_free(pixels);
    *dataOut = rgba;
    return 0;
}

int BgfxRenderPath::LoadTextureData(uint8_t* data, uint32_t bytes, void* srcInfo, int** dataOut) {
    int w, h, channels;
    unsigned char* pixels = stbi_load_from_memory(data, bytes, &w, &h, &channels, 4);
    if (!pixels) return -1;
    auto* info = static_cast<D3DXIMAGE_INFO*>(srcInfo);
    if (info) { info->Width = w; info->Height = h; }
    int* rgba = new int[w * h];
    memcpy(rgba, pixels, w * h * 4);
    stbi_image_free(pixels);
    *dataOut = rgba;
    return 0;
}

// -- Frame lifecycle --------------------------------------------------------

void BgfxRenderPath::StartFrame() {
    int w, h;
    SDL_GetWindowSize(window_, &w, &h);
    width_ = w; height_ = h;
    fb_.width = w; fb_.height = h;
    fb_.aspect = h > 0 ? (float)w / (float)h : 1.0f;
    fb_.is_widescreen = fb_.aspect > 1.5f;
    fb_.is_hi_def = h >= 720;
}
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

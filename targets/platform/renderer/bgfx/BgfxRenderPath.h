#pragma once

#include <stack>
#include <vector>

#include <glm/glm.hpp>

#include <bgfx/bgfx.h>

#include "platform/renderer/IRenderPath.h"

struct SDL_Window;

class BgfxRenderPath final : public rp::IRenderPath {
public:
    explicit BgfxRenderPath(SDL_Window* window);
    ~BgfxRenderPath() override;

    // -- New surface --------------------------------------------------------

    [[nodiscard]] rp::MeshHandle    create_mesh(const rp::MeshDesc& desc) override;
    void                            update_mesh(rp::MeshHandle h, const rp::MeshDesc& desc) override;
    void                            destroy_mesh(rp::MeshHandle h) override;

    [[nodiscard]] rp::TextureHandle create_texture(const rp::TextureDesc& desc) override;
    void                            update_texture(rp::TextureHandle h, const rp::TextureRegion& region) override;
    void                            destroy_texture(rp::TextureHandle h) override;

    [[nodiscard]] rp::MaterialHandle create_material(const rp::MaterialDesc& desc) override;
    void                             update_material(rp::MaterialHandle h, const rp::MaterialDesc& desc) override;
    void                             destroy_material(rp::MaterialHandle h) override;

    [[nodiscard]] std::pair<rp::TransientVertexBuffer, std::span<std::byte>>
        alloc_transient_vertices(uint32_t vertex_count, rp::VertexLayout layout,
                                 rp::PrimitiveType primitive) override;

    void render_frame(const rp::FrameDesc& frame) override;
    void resize(uint32_t w, uint32_t h) override;
    [[nodiscard]] const rp::FrameFramebuffer& framebuffer() const override;
    void read_framebuffer(const rp::TextureReadback& req) override;
    [[nodiscard]] rp::ResourceFootprint query_resource_footprint() const override;
    void seal_static_resource_tier() override;
    void begin_atomic_resource_batch() override;
    void end_atomic_resource_batch() override;
    void push_debug_event(const char* name) override;
    void pop_debug_event() override;
    void tick() override;

    // -- Legacy state methods (software emulation) --------------------------

    void MatrixMode(rp::MatrixStack stack) override;
    void MatrixSetIdentity() override;
    void MatrixTranslate(float x, float y, float z) override;
    void MatrixRotate(float angle, float x, float y, float z) override;
    void MatrixScale(float x, float y, float z) override;
    void MatrixPerspective(float fovy, float aspect, float zNear, float zFar) override;
    void MatrixOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar) override;
    void MatrixPop() override;
    void MatrixPush() override;
    void MatrixMult(float* mat) override;
    [[nodiscard]] const float* MatrixGet(rp::MatrixStack stack) override;

    void DrawVertices(int primitiveType, int count, void* data,
                      int vertexType, int shaderType) override;

    [[nodiscard]] int CBuffCreate(int count) override;
    void CBuffDelete(int first, int count) override;
    void CBuffDeleteAll() override;
    void CBuffStart(int index, bool full) override;
    void CBuffClear(int index) override;
    [[nodiscard]] int CBuffSize(int index) override;
    void CBuffEnd() override;
    [[nodiscard]] bool CBuffCall(int index, bool full) override;
    void CBuffDeferredModeStart() override;
    void CBuffDeferredModeEnd() override;

    [[nodiscard]] int TextureCreate() override;
    void TextureFree(int idx) override;
    void TextureBind(int idx) override;
    void TextureBindVertex(int idx, bool scaleLight) override;
    void TextureSetTextureLevels(int levels) override;
    void TextureData(int width, int height, void* data, int level, int format) override;
    void TextureDataUpdate(int xoff, int yoff, int w, int h, void* data, int level) override;
    void TextureSetParam(int param, int value) override;

    void StateSetColour(float r, float g, float b, float a) override;
    void StateSetDepthMask(bool enable) override;
    void StateSetBlendEnable(bool enable) override;
    void StateSetBlendFunc(rp::BlendFactor src, rp::BlendFactor dst) override;
    void StateSetBlendFactor(unsigned int colour) override;
    void StateSetAlphaFunc(rp::AlphaTest func, float param) override;
    void StateSetDepthFunc(rp::DepthTest func) override;
    void StateSetFaceCull(bool enable) override;
    void StateSetLineWidth(float width) override;
    void StateSetWriteEnable(bool r, bool g, bool b, bool a) override;
    void StateSetDepthTestEnable(bool enable) override;
    void StateSetAlphaTestEnable(bool enable) override;
    void StateSetDepthSlopeAndBias(float slope, float bias) override;

    void StateSetFogEnable(bool enable) override;
    void StateSetFogMode(rp::FogMode mode) override;
    void StateSetFogNearDistance(float dist) override;
    void StateSetFogFarDistance(float dist) override;
    void StateSetFogDensity(float density) override;
    void StateSetFogColour(float r, float g, float b) override;

    void StateSetLightingEnable(bool enable) override;
    void StateSetLightColour(int light, float r, float g, float b) override;
    void StateSetLightAmbientColour(float r, float g, float b) override;
    void StateSetLightDirection(int light, float x, float y, float z) override;
    void StateSetLightEnable(int light, bool enable) override;

    void StateSetViewport(int viewportType) override;
    void StateSetEnableViewportClipPlanes(bool enable) override;
    void StateSetStencil(int func, uint8_t ref, uint8_t funcMask, uint8_t writeMask) override;
    void StateSetForceLOD(int lod) override;
    void StateSetTextureEnable(bool enable) override;
    void StateSetActiveTexture(int tex) override;

    void SetChunkOffset(float x, float y, float z) override;

    [[nodiscard]] int TextureGetTextureLevels() override;
    void ReadPixels(int x, int y, int w, int h, void* buf) override;
    [[nodiscard]] int LoadTextureData(const char* filename, void* srcInfo, int** dataOut) override;
    [[nodiscard]] int LoadTextureData(uint8_t* data, uint32_t bytes, void* srcInfo, int** dataOut) override;

    void StateSetVertexTextureUV(float u, float v) override;

    void StartFrame() override;
    void Present() override;
    void Clear(int flags) override;
    void SetClearColour(const float rgba[4]) override;
    void Set_matrixDirty() override;
    void CBuffLockStaticCreations() override;

    void GetFramebufferSize(int& w, int& h) override;
    [[nodiscard]] bool IsWidescreen() override;
    [[nodiscard]] bool IsHiDef() override;
    void Close() override;
    [[nodiscard]] bool ShouldClose() override;
    void SetWindowSize(int w, int h) override;
    void SetFullscreen(bool fs) override;
    void UpdateGamma(unsigned short gamma) override;
    void Suspend() override;
    [[nodiscard]] bool Suspended() override;
    void Resume() override;

    void BeginEvent(const char* name) override;
    void EndEvent() override;

    void submit_immediate(const rp::DrawCall& dc) override;

private:
    std::stack<glm::mat4>& current_stack();

    SDL_Window* window_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    rp::FrameFramebuffer fb_{};
    bool should_close_ = false;

    // Software matrix stack
    rp::MatrixStack matrix_mode_ = rp::MatrixStack::modelview; // GL_MODELVIEW
    std::stack<glm::mat4> projection_stack_;
    std::stack<glm::mat4> modelview_stack_;
    glm::mat4 projection_top_{1.0f};
    glm::mat4 modelview_top_{1.0f};

    // State accumulator
    uint64_t bgfx_state_ = 0;
    float tint_color_[4] = {1, 1, 1, 1};
    bool blend_enabled_ = false;
    bool depth_test_enabled_ = true;
    bool depth_write_ = true;
    bool cull_enabled_ = true;
    bool texture_enabled_ = true;
    int bound_texture_ = -1;
    float chunk_offset_[3] = {0, 0, 0};

    // bgfx vertex layouts
    bgfx::VertexLayout vl_world_standard_;

    // bgfx shader program
    bgfx::ProgramHandle program_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_tintColor_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_params_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle s_texColor_ = BGFX_INVALID_HANDLE;

    // View ID management
    uint16_t current_view_id_ = 0;

    // Transient arena
    std::vector<std::byte> transient_arena_;
    uint32_t transient_offset_ = 0;
    uint32_t current_frame_ = 0;

    // Material storage
    struct MaterialSlot {
        rp::MaterialDesc desc;
        uint32_t generation = 0;
        bool occupied = false;
    };
    std::vector<MaterialSlot> materials_;

    // Texture storage (maps our handles to bgfx handles)
    struct TextureSlot {
        bgfx::TextureHandle bgfx_handle = BGFX_INVALID_HANDLE;
        uint32_t generation = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        bool occupied = false;
    };
    std::vector<TextureSlot> textures_;

    // GL texture ID to our handle map (for legacy TextureCreate/Bind)
    std::vector<bgfx::TextureHandle> gl_tex_map_;
};

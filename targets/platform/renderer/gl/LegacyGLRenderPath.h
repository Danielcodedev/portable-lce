#pragma once

#include "platform/renderer/IRenderPath.h"

class LegacyGLRenderPath final : public rp::IRenderPath {
public:
    LegacyGLRenderPath();
    ~LegacyGLRenderPath() override;

    // -- New surface (stubs / translations to PlatformRenderer) -------------

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
    void read_framebuffer(const rp::TextureReadback& req) override;
    [[nodiscard]] rp::ResourceFootprint query_resource_footprint() const override;
    void seal_static_resource_tier() override;
    void begin_atomic_resource_batch() override;
    void end_atomic_resource_batch() override;
    void push_debug_event(const char* name) override;
    void pop_debug_event() override;
    void tick() override;

    // -- Legacy forwards (all [[deprecated]] in IRenderPath) ----------------

    void MatrixMode(int type) override;
    void MatrixSetIdentity() override;
    void MatrixTranslate(float x, float y, float z) override;
    void MatrixRotate(float angle, float x, float y, float z) override;
    void MatrixScale(float x, float y, float z) override;
    void MatrixPerspective(float fovy, float aspect, float zNear, float zFar) override;
    void MatrixOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar) override;
    void MatrixPop() override;
    void MatrixPush() override;
    void MatrixMult(float* mat) override;
    [[nodiscard]] const float* MatrixGet(int type) override;

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
    void StateSetBlendFunc(int src, int dst) override;
    void StateSetBlendFactor(unsigned int colour) override;
    void StateSetAlphaFunc(int func, float param) override;
    void StateSetDepthFunc(int func) override;
    void StateSetFaceCull(bool enable) override;
    void StateSetLineWidth(float width) override;
    void StateSetWriteEnable(bool r, bool g, bool b, bool a) override;
    void StateSetDepthTestEnable(bool enable) override;
    void StateSetAlphaTestEnable(bool enable) override;

    void StateSetFogEnable(bool enable) override;
    void StateSetFogMode(int mode) override;
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

    void GetFramebufferSize(int& w, int& h) override;
    [[nodiscard]] bool IsWidescreen() override;
    [[nodiscard]] bool IsHiDef() override;

    [[nodiscard]] int TextureGetTextureLevels() override;
    void TextureDynamicUpdateStart() override;
    void TextureDynamicUpdateEnd() override;
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
    void execute_draw(const rp::DrawCall& dc);
    void execute_chunk_draw(const rp::ChunkDrawCall& cdc);
    void apply_material(const rp::MaterialDesc& mat);

    int atomic_batch_depth_ = 0;
    bool static_tier_sealed_ = false;

    // Transient vertex arena - reset each frame.
    std::vector<std::byte> transient_arena_;
    uint32_t transient_offset_ = 0;
    uint32_t current_frame_ = 0;

    // Material storage (flat vector, generational).
    struct MaterialSlot {
        rp::MaterialDesc desc;
        uint32_t generation = 0;
        bool occupied = false;
    };
    std::vector<MaterialSlot> materials_;

    // Mesh storage - maps MeshHandle to a CBuffer slot.
    struct MeshSlot {
        int cbuff_id = -1;
        uint32_t generation = 0;
        bool occupied = false;
    };
    std::vector<MeshSlot> meshes_;
};

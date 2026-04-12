#include "LegacyGLRenderPath.h"

#include <cassert>

#include "platform/renderer/gl/gl_compat.h"
#include "platform/renderer/renderer.h"

using namespace rp;

// ---------------------------------------------------------------------------
// Global render path accessor storage
// ---------------------------------------------------------------------------

namespace rp::render_path_internal {

static IRenderPath* s_active = nullptr;

void set_active(IRenderPath* path) { s_active = path; }

IRenderPath& get_active() {
    assert(s_active && "RenderPath accessed before set_active()");
    return *s_active;
}

} // namespace rp::render_path_internal

// Suppress deprecation warnings for the legacy section - this file IS the
// forwarding implementation.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

static constexpr uint32_t TRANSIENT_ARENA_SIZE = 16 * 1024 * 1024;

LegacyGLRenderPath::LegacyGLRenderPath() {
    transient_arena_.resize(TRANSIENT_ARENA_SIZE);
}

LegacyGLRenderPath::~LegacyGLRenderPath() = default;

// ---------------------------------------------------------------------------
// New surface implementations
// ---------------------------------------------------------------------------

static void upload_mesh_to_cbuff(int cbuff_id, const MeshDesc& desc) {
    if (desc.vertex_count == 0 || desc.vertex_data.empty()) {
        PlatformRenderer.CBuffClear(cbuff_id);
        return;
    }
    auto vtype = IPlatformRenderer::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1;
    switch (desc.layout) {
        case VertexLayout::chunk_compact:  vtype = IPlatformRenderer::VERTEX_TYPE_COMPRESSED; break;
        case VertexLayout::world_standard: vtype = IPlatformRenderer::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1; break;
        case VertexLayout::world_texgen:   vtype = IPlatformRenderer::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN; break;
    }
    auto ptype = IPlatformRenderer::PRIMITIVE_TYPE_TRIANGLE_LIST;
    switch (desc.primitive) {
        case PrimitiveType::triangle_list:  ptype = IPlatformRenderer::PRIMITIVE_TYPE_TRIANGLE_LIST; break;
        case PrimitiveType::triangle_strip: ptype = IPlatformRenderer::PRIMITIVE_TYPE_TRIANGLE_STRIP; break;
        case PrimitiveType::triangle_fan:   ptype = IPlatformRenderer::PRIMITIVE_TYPE_TRIANGLE_FAN; break;
        case PrimitiveType::line_list:      ptype = IPlatformRenderer::PRIMITIVE_TYPE_LINE_LIST; break;
        case PrimitiveType::line_strip:     ptype = IPlatformRenderer::PRIMITIVE_TYPE_LINE_STRIP; break;
    }
    PlatformRenderer.CBuffStart(cbuff_id, true);
    PlatformRenderer.DrawVertices(ptype, desc.vertex_count,
                                  const_cast<void*>(static_cast<const void*>(desc.vertex_data.data())),
                                  vtype, IPlatformRenderer::PIXEL_SHADER_TYPE_STANDARD);
    PlatformRenderer.CBuffEnd();
}

MeshHandle LegacyGLRenderPath::create_mesh(const MeshDesc& desc) {
    int cbuff_id = PlatformRenderer.CBuffCreate(1);
    if (cbuff_id < 0) return kInvalidMesh;

    upload_mesh_to_cbuff(cbuff_id, desc);

    for (uint32_t i = 0; i < meshes_.size(); ++i) {
        if (!meshes_[i].occupied) {
            meshes_[i].cbuff_id = cbuff_id;
            meshes_[i].generation++;
            meshes_[i].occupied = true;
            return {i, meshes_[i].generation};
        }
    }
    uint32_t idx = static_cast<uint32_t>(meshes_.size());
    meshes_.push_back({cbuff_id, 1, true});
    return {idx, 1};
}

void LegacyGLRenderPath::update_mesh(MeshHandle h, const MeshDesc& desc) {
    if (h.index >= meshes_.size() || meshes_[h.index].generation != h.generation)
        return;
    upload_mesh_to_cbuff(meshes_[h.index].cbuff_id, desc);
}

void LegacyGLRenderPath::destroy_mesh(MeshHandle h) {
    if (h.index >= meshes_.size() || meshes_[h.index].generation != h.generation)
        return;
    PlatformRenderer.CBuffDelete(meshes_[h.index].cbuff_id, 1);
    meshes_[h.index].occupied = false;
}

TextureHandle LegacyGLRenderPath::create_texture(const TextureDesc&) {
    return kInvalidTexture;
}

void LegacyGLRenderPath::update_texture(TextureHandle, const TextureRegion&) {}
void LegacyGLRenderPath::destroy_texture(TextureHandle) {}

MaterialHandle LegacyGLRenderPath::create_material(const MaterialDesc& desc) {
    for (uint32_t i = 0; i < materials_.size(); ++i) {
        if (!materials_[i].occupied) {
            materials_[i].desc = desc;
            materials_[i].generation++;
            materials_[i].occupied = true;
            return {i, materials_[i].generation};
        }
    }
    uint32_t idx = static_cast<uint32_t>(materials_.size());
    materials_.push_back({desc, 1, true});
    return {idx, 1};
}

void LegacyGLRenderPath::update_material(MaterialHandle h, const MaterialDesc& desc) {
    if (h.index < materials_.size() && materials_[h.index].generation == h.generation)
        materials_[h.index].desc = desc;
}

void LegacyGLRenderPath::destroy_material(MaterialHandle h) {
    if (h.index < materials_.size() && materials_[h.index].generation == h.generation)
        materials_[h.index].occupied = false;
}

std::pair<TransientVertexBuffer, std::span<std::byte>>
LegacyGLRenderPath::alloc_transient_vertices(uint32_t vertex_count, VertexLayout layout,
                                              PrimitiveType primitive) {
    uint32_t stride = 0;
    switch (layout) {
        case VertexLayout::chunk_compact:  stride = 16; break;
        case VertexLayout::world_standard: stride = 32; break;
        case VertexLayout::world_texgen:   stride = 32; break;
    }
    uint32_t bytes = vertex_count * stride;
    if (transient_offset_ + bytes > transient_arena_.size())
        return {{}, {}};

    TransientVertexBuffer tvb;
    tvb.frame_index  = current_frame_;
    tvb.offset       = transient_offset_;
    tvb.vertex_count = vertex_count;
    tvb.layout       = layout;
    tvb.primitive    = primitive;

    auto span = std::span<std::byte>(transient_arena_.data() + transient_offset_, bytes);
    transient_offset_ += bytes;
    return {tvb, span};
}

// ---------------------------------------------------------------------------
// render_frame - translates FrameDesc back into PlatformRenderer calls
// ---------------------------------------------------------------------------

void LegacyGLRenderPath::apply_material(const MaterialDesc& mat) {
    PlatformRenderer.StateSetTextureEnable(mat.textured);
    PlatformRenderer.StateSetLightingEnable(mat.lit);
    PlatformRenderer.StateSetFogEnable(mat.fog_enabled);

    switch (mat.depth_test) {
        case DepthTest::off:
            PlatformRenderer.StateSetDepthTestEnable(false);
            break;
        default:
            PlatformRenderer.StateSetDepthTestEnable(true);
            PlatformRenderer.StateSetDepthFunc(static_cast<int>(mat.depth_test));
            break;
    }
    PlatformRenderer.StateSetDepthMask(mat.depth_write);

    switch (mat.blend) {
        case BlendMode::opaque:
            PlatformRenderer.StateSetBlendEnable(false);
            break;
        case BlendMode::alpha:
            PlatformRenderer.StateSetBlendEnable(true);
            PlatformRenderer.StateSetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::additive:
            PlatformRenderer.StateSetBlendEnable(true);
            PlatformRenderer.StateSetBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BlendMode::multiply:
            PlatformRenderer.StateSetBlendEnable(true);
            PlatformRenderer.StateSetBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
        case BlendMode::premultiplied:
            PlatformRenderer.StateSetBlendEnable(true);
            PlatformRenderer.StateSetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::custom:
            PlatformRenderer.StateSetBlendEnable(true);
            PlatformRenderer.StateSetBlendFunc(
                static_cast<int>(mat.blend_src_custom),
                static_cast<int>(mat.blend_dst_custom));
            break;
    }

    if (mat.alpha_test != AlphaTest::off) {
        PlatformRenderer.StateSetAlphaTestEnable(true);
        PlatformRenderer.StateSetAlphaFunc(static_cast<int>(mat.alpha_test), mat.alpha_ref);
    } else {
        PlatformRenderer.StateSetAlphaTestEnable(false);
    }

    PlatformRenderer.StateSetFaceCull(mat.cull != CullMode::none);
}

void LegacyGLRenderPath::execute_draw(const DrawCall& dc) {
    if (dc.material && dc.material.index < materials_.size() &&
        materials_[dc.material.index].generation == dc.material.generation) {
        apply_material(materials_[dc.material.index].desc);
    }

    PlatformRenderer.StateSetColour(dc.tint_color[0], dc.tint_color[1],
                                    dc.tint_color[2], dc.tint_color[3]);

    if (dc.texture_override) {
        if (dc.texture_override.generation == 1) {
            PlatformRenderer.TextureBind(static_cast<int>(dc.texture_override.index));
        }
    }

    if (dc.blend_constant_factor != 0xFFFFFFFF)
        PlatformRenderer.StateSetBlendFactor(dc.blend_constant_factor);

    if (dc.line_width > 0)
        PlatformRenderer.StateSetLineWidth(dc.line_width);

    if (dc.source == VertexSource::mesh) {
        const auto& mh = dc.mesh;
        if (mh.index < meshes_.size() &&
            meshes_[mh.index].generation == mh.generation &&
            meshes_[mh.index].occupied) {
            PlatformRenderer.CBuffCall(meshes_[mh.index].cbuff_id, true);
        }
    } else {
        const auto& tvb = dc.transient;
        if (tvb.frame_index != current_frame_) return;
        void* data = transient_arena_.data() + tvb.offset;

        auto vtype = IPlatformRenderer::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1;
        switch (tvb.layout) {
            case VertexLayout::chunk_compact:  vtype = IPlatformRenderer::VERTEX_TYPE_COMPRESSED; break;
            case VertexLayout::world_standard: vtype = IPlatformRenderer::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1; break;
            case VertexLayout::world_texgen:   vtype = IPlatformRenderer::VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN; break;
        }

        auto ptype = IPlatformRenderer::PRIMITIVE_TYPE_TRIANGLE_LIST;
        switch (tvb.primitive) {
            case PrimitiveType::triangle_list:  ptype = IPlatformRenderer::PRIMITIVE_TYPE_TRIANGLE_LIST; break;
            case PrimitiveType::triangle_strip: ptype = IPlatformRenderer::PRIMITIVE_TYPE_TRIANGLE_STRIP; break;
            case PrimitiveType::triangle_fan:   ptype = IPlatformRenderer::PRIMITIVE_TYPE_TRIANGLE_FAN; break;
            case PrimitiveType::line_list:      ptype = IPlatformRenderer::PRIMITIVE_TYPE_LINE_LIST; break;
            case PrimitiveType::line_strip:     ptype = IPlatformRenderer::PRIMITIVE_TYPE_LINE_STRIP; break;
        }

        PlatformRenderer.DrawVertices(ptype, tvb.vertex_count, data, vtype,
                                      IPlatformRenderer::PIXEL_SHADER_TYPE_STANDARD);
    }
}

void LegacyGLRenderPath::execute_chunk_draw(const rp::ChunkDrawCall& cdc) {
    if (cdc.mesh.index < meshes_.size() &&
        meshes_[cdc.mesh.index].generation == cdc.mesh.generation &&
        meshes_[cdc.mesh.index].occupied) {
        PlatformRenderer.SetChunkOffset(cdc.chunk_offset[0],
                                        cdc.chunk_offset[1],
                                        cdc.chunk_offset[2]);
        PlatformRenderer.CBuffCall(meshes_[cdc.mesh.index].cbuff_id, true);
    }
}

const FrameFramebuffer& LegacyGLRenderPath::framebuffer() const {
    return current_framebuffer_;
}

void LegacyGLRenderPath::render_frame(const FrameDesc& frame) {
    transient_offset_ = 0;
    current_frame_++;
    current_framebuffer_ = frame.framebuffer;

    for (const auto& view : frame.views) {
        for (const auto& cdc : view.chunk_opaque)      execute_chunk_draw(cdc);
        for (const auto& cdc : view.chunk_alpha_test)   execute_chunk_draw(cdc);
        for (const auto& cdc : view.chunk_transparent)  execute_chunk_draw(cdc);
        for (const auto& dc : view.world_opaque)        execute_draw(dc);
        for (const auto& dc : view.world_alpha_test)    execute_draw(dc);
        for (const auto& dc : view.world_transparent)   execute_draw(dc);
        for (const auto& dc : view.debug_overlay)       execute_draw(dc);
    }

    for (const auto& dc : frame.ui_overlay)
        execute_draw(dc);
}

void LegacyGLRenderPath::resize(uint32_t, uint32_t) {}

void LegacyGLRenderPath::read_framebuffer(const TextureReadback&) {}

ResourceFootprint LegacyGLRenderPath::query_resource_footprint() const {
    return {0, 0, static_cast<uint64_t>(PlatformRenderer.CBuffSize(-1))};
}

void LegacyGLRenderPath::seal_static_resource_tier() {
    if (!static_tier_sealed_) {
        PlatformRenderer.CBuffLockStaticCreations();
        static_tier_sealed_ = true;
    }
}

void LegacyGLRenderPath::begin_atomic_resource_batch() {
    // Atomic batches delegate to CBuffDeferredMode which is main-thread-only
    // in the GL backend. A future backend with real thread-transparent
    // resource creation would use a thread-safe counter here.
    if (atomic_batch_depth_++ == 0)
        PlatformRenderer.CBuffDeferredModeStart();
}

void LegacyGLRenderPath::end_atomic_resource_batch() {
    assert(atomic_batch_depth_ > 0);
    if (--atomic_batch_depth_ == 0)
        PlatformRenderer.CBuffDeferredModeEnd();
}

void LegacyGLRenderPath::push_debug_event(const char* name) {
    PlatformRenderer.BeginEvent(name);
}

void LegacyGLRenderPath::pop_debug_event() {
    PlatformRenderer.EndEvent();
}

void LegacyGLRenderPath::tick() {
    PlatformRenderer.Tick();
}

// ---------------------------------------------------------------------------
// Legacy forwards - each delegates to the PlatformRenderer singleton.
// ---------------------------------------------------------------------------

void LegacyGLRenderPath::MatrixMode(int type)                    { PlatformRenderer.MatrixMode(type); }
void LegacyGLRenderPath::MatrixSetIdentity()                     { PlatformRenderer.MatrixSetIdentity(); }
void LegacyGLRenderPath::MatrixTranslate(float x, float y, float z) { PlatformRenderer.MatrixTranslate(x, y, z); }
void LegacyGLRenderPath::MatrixRotate(float a, float x, float y, float z) { PlatformRenderer.MatrixRotate(a, x, y, z); }
void LegacyGLRenderPath::MatrixScale(float x, float y, float z)  { PlatformRenderer.MatrixScale(x, y, z); }
void LegacyGLRenderPath::MatrixPerspective(float fovy, float aspect, float zn, float zf) { PlatformRenderer.MatrixPerspective(fovy, aspect, zn, zf); }
void LegacyGLRenderPath::MatrixOrthogonal(float l, float r, float b, float t, float zn, float zf) { PlatformRenderer.MatrixOrthogonal(l, r, b, t, zn, zf); }
void LegacyGLRenderPath::MatrixPop()                             { PlatformRenderer.MatrixPop(); }
void LegacyGLRenderPath::MatrixPush()                            { PlatformRenderer.MatrixPush(); }
void LegacyGLRenderPath::MatrixMult(float* m)                    { PlatformRenderer.MatrixMult(m); }
const float* LegacyGLRenderPath::MatrixGet(int type)             { return PlatformRenderer.MatrixGet(type); }

void LegacyGLRenderPath::DrawVertices(int prim, int count, void* data, int vtype, int stype) {
    PlatformRenderer.DrawVertices(
        static_cast<IPlatformRenderer::ePrimitiveType>(prim), count, data,
        static_cast<IPlatformRenderer::eVertexType>(vtype),
        static_cast<IPlatformRenderer::ePixelShaderType>(stype));
}

int  LegacyGLRenderPath::CBuffCreate(int count)                  { return PlatformRenderer.CBuffCreate(count); }
void LegacyGLRenderPath::CBuffDelete(int first, int count)       { PlatformRenderer.CBuffDelete(first, count); }
void LegacyGLRenderPath::CBuffDeleteAll()                        { PlatformRenderer.CBuffDeleteAll(); }
void LegacyGLRenderPath::CBuffStart(int index, bool full)        { PlatformRenderer.CBuffStart(index, full); }
void LegacyGLRenderPath::CBuffClear(int index)                   { PlatformRenderer.CBuffClear(index); }
int  LegacyGLRenderPath::CBuffSize(int index)                    { return PlatformRenderer.CBuffSize(index); }
void LegacyGLRenderPath::CBuffEnd()                              { PlatformRenderer.CBuffEnd(); }
bool LegacyGLRenderPath::CBuffCall(int index, bool full)         { return PlatformRenderer.CBuffCall(index, full); }
void LegacyGLRenderPath::CBuffDeferredModeStart()                { PlatformRenderer.CBuffDeferredModeStart(); }
void LegacyGLRenderPath::CBuffDeferredModeEnd()                  { PlatformRenderer.CBuffDeferredModeEnd(); }

int  LegacyGLRenderPath::TextureCreate()                         { return PlatformRenderer.TextureCreate(); }
void LegacyGLRenderPath::TextureFree(int idx)                    { PlatformRenderer.TextureFree(idx); }
void LegacyGLRenderPath::TextureBind(int idx)                    { PlatformRenderer.TextureBind(idx); }
void LegacyGLRenderPath::TextureBindVertex(int idx, bool sl)     { PlatformRenderer.TextureBindVertex(idx, sl); }
void LegacyGLRenderPath::TextureSetTextureLevels(int levels)     { PlatformRenderer.TextureSetTextureLevels(levels); }
void LegacyGLRenderPath::TextureData(int w, int h, void* d, int l, int f) {
    PlatformRenderer.TextureData(w, h, d, l, static_cast<IPlatformRenderer::eTextureFormat>(f));
}
void LegacyGLRenderPath::TextureDataUpdate(int xo, int yo, int w, int h, void* d, int l) { PlatformRenderer.TextureDataUpdate(xo, yo, w, h, d, l); }
void LegacyGLRenderPath::TextureSetParam(int p, int v)           { PlatformRenderer.TextureSetParam(p, v); }

void LegacyGLRenderPath::StateSetColour(float r, float g, float b, float a) { PlatformRenderer.StateSetColour(r, g, b, a); }
void LegacyGLRenderPath::StateSetDepthMask(bool e)               { PlatformRenderer.StateSetDepthMask(e); }
void LegacyGLRenderPath::StateSetBlendEnable(bool e)             { PlatformRenderer.StateSetBlendEnable(e); }
void LegacyGLRenderPath::StateSetBlendFunc(int s, int d)         { PlatformRenderer.StateSetBlendFunc(s, d); }
void LegacyGLRenderPath::StateSetBlendFactor(unsigned int c)     { PlatformRenderer.StateSetBlendFactor(c); }
void LegacyGLRenderPath::StateSetAlphaFunc(int f, float p)       { PlatformRenderer.StateSetAlphaFunc(f, p); }
void LegacyGLRenderPath::StateSetDepthFunc(int f)                { PlatformRenderer.StateSetDepthFunc(f); }
void LegacyGLRenderPath::StateSetFaceCull(bool e)                { PlatformRenderer.StateSetFaceCull(e); }
void LegacyGLRenderPath::StateSetLineWidth(float w)              { PlatformRenderer.StateSetLineWidth(w); }
void LegacyGLRenderPath::StateSetWriteEnable(bool r, bool g, bool b, bool a) { PlatformRenderer.StateSetWriteEnable(r, g, b, a); }
void LegacyGLRenderPath::StateSetDepthTestEnable(bool e)         { PlatformRenderer.StateSetDepthTestEnable(e); }
void LegacyGLRenderPath::StateSetAlphaTestEnable(bool e)         { PlatformRenderer.StateSetAlphaTestEnable(e); }

void LegacyGLRenderPath::StateSetFogEnable(bool e)               { PlatformRenderer.StateSetFogEnable(e); }
void LegacyGLRenderPath::StateSetFogMode(int m)                  { PlatformRenderer.StateSetFogMode(m); }
void LegacyGLRenderPath::StateSetFogNearDistance(float d)        { PlatformRenderer.StateSetFogNearDistance(d); }
void LegacyGLRenderPath::StateSetFogFarDistance(float d)         { PlatformRenderer.StateSetFogFarDistance(d); }
void LegacyGLRenderPath::StateSetFogDensity(float d)             { PlatformRenderer.StateSetFogDensity(d); }
void LegacyGLRenderPath::StateSetFogColour(float r, float g, float b) { PlatformRenderer.StateSetFogColour(r, g, b); }

void LegacyGLRenderPath::StateSetLightingEnable(bool e)          { PlatformRenderer.StateSetLightingEnable(e); }
void LegacyGLRenderPath::StateSetLightColour(int l, float r, float g, float b) { PlatformRenderer.StateSetLightColour(l, r, g, b); }
void LegacyGLRenderPath::StateSetLightAmbientColour(float r, float g, float b) { PlatformRenderer.StateSetLightAmbientColour(r, g, b); }
void LegacyGLRenderPath::StateSetLightDirection(int l, float x, float y, float z) { PlatformRenderer.StateSetLightDirection(l, x, y, z); }
void LegacyGLRenderPath::StateSetLightEnable(int l, bool e)      { PlatformRenderer.StateSetLightEnable(l, e); }

void LegacyGLRenderPath::StateSetViewport(int vt)                { PlatformRenderer.StateSetViewport(static_cast<IPlatformRenderer::eViewportType>(vt)); }
void LegacyGLRenderPath::StateSetEnableViewportClipPlanes(bool e){ PlatformRenderer.StateSetEnableViewportClipPlanes(e); }
void LegacyGLRenderPath::StateSetStencil(int f, uint8_t r, uint8_t fm, uint8_t wm) { PlatformRenderer.StateSetStencil(f, r, fm, wm); }
void LegacyGLRenderPath::StateSetForceLOD(int l)                 { PlatformRenderer.StateSetForceLOD(l); }
void LegacyGLRenderPath::StateSetTextureEnable(bool e)           { PlatformRenderer.StateSetTextureEnable(e); }
void LegacyGLRenderPath::StateSetActiveTexture(int t)            { PlatformRenderer.StateSetActiveTexture(t); }

void LegacyGLRenderPath::SetChunkOffset(float x, float y, float z) { PlatformRenderer.SetChunkOffset(x, y, z); }

void LegacyGLRenderPath::GetFramebufferSize(int& w, int& h)     { PlatformRenderer.GetFramebufferSize(w, h); }
bool LegacyGLRenderPath::IsWidescreen()                          { return PlatformRenderer.IsWidescreen(); }
bool LegacyGLRenderPath::IsHiDef()                               { return PlatformRenderer.IsHiDef(); }

int  LegacyGLRenderPath::TextureGetTextureLevels()               { return PlatformRenderer.TextureGetTextureLevels(); }
void LegacyGLRenderPath::ReadPixels(int x, int y, int w, int h, void* b) { PlatformRenderer.ReadPixels(x, y, w, h, b); }
int  LegacyGLRenderPath::LoadTextureData(const char* f, void* si, int** d) { return PlatformRenderer.LoadTextureData(f, static_cast<D3DXIMAGE_INFO*>(si), d); }
int  LegacyGLRenderPath::LoadTextureData(uint8_t* data, uint32_t bytes, void* si, int** d) { return PlatformRenderer.LoadTextureData(data, bytes, static_cast<D3DXIMAGE_INFO*>(si), d); }

void LegacyGLRenderPath::StateSetVertexTextureUV(float u, float v) { PlatformRenderer.StateSetVertexTextureUV(u, v); }

void LegacyGLRenderPath::StartFrame() {
    PlatformRenderer.StartFrame();
    int w = 0, h = 0;
    PlatformRenderer.GetFramebufferSize(w, h);
    current_framebuffer_.width  = w;
    current_framebuffer_.height = h;
    current_framebuffer_.aspect = h > 0 ? (float)w / (float)h : 1.0f;
    current_framebuffer_.is_widescreen = PlatformRenderer.IsWidescreen();
    current_framebuffer_.is_hi_def     = PlatformRenderer.IsHiDef();
}
void LegacyGLRenderPath::Present()                               { PlatformRenderer.Present(); }
void LegacyGLRenderPath::Clear(int f)                            { PlatformRenderer.Clear(f); }
void LegacyGLRenderPath::SetClearColour(const float c[4])        { PlatformRenderer.SetClearColour(c); }
void LegacyGLRenderPath::Set_matrixDirty()                       { PlatformRenderer.Set_matrixDirty(); }
void LegacyGLRenderPath::CBuffLockStaticCreations()              { PlatformRenderer.CBuffLockStaticCreations(); }

void LegacyGLRenderPath::Close()                                 { PlatformRenderer.Close(); }
bool LegacyGLRenderPath::ShouldClose()                           { return PlatformRenderer.ShouldClose(); }
void LegacyGLRenderPath::SetWindowSize(int w, int h)             { PlatformRenderer.SetWindowSize(w, h); }
void LegacyGLRenderPath::SetFullscreen(bool f)                   { PlatformRenderer.SetFullscreen(f); }
void LegacyGLRenderPath::UpdateGamma(unsigned short g)           { PlatformRenderer.UpdateGamma(g); }
void LegacyGLRenderPath::Suspend()                               { PlatformRenderer.Suspend(); }
bool LegacyGLRenderPath::Suspended()                             { return PlatformRenderer.Suspended(); }
void LegacyGLRenderPath::Resume()                                { PlatformRenderer.Resume(); }

void LegacyGLRenderPath::BeginEvent(const char* n)               { PlatformRenderer.BeginEvent(n); }
void LegacyGLRenderPath::EndEvent()                              { PlatformRenderer.EndEvent(); }

void LegacyGLRenderPath::submit_immediate(const DrawCall& dc)    { execute_draw(dc); }

#pragma GCC diagnostic pop

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::unique_ptr<IRenderPath> make_legacy_gl_render_path() {
    return std::make_unique<LegacyGLRenderPath>();
}

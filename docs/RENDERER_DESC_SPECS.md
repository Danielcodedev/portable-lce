# Renderer Desc Specs

## 0. Purpose and scope

This document is Phase 1 of the renderer refactor. It defines the data
structures that Phase 2 will expose as the new `IRenderPath` interface:

- Handle types (`MeshHandle`, `TextureHandle`, `MaterialHandle`).
- Resource descriptors: `TextureDesc`, `MeshDesc`, `MaterialDesc`.
- Per-frame submission: `DrawCall`, `ViewDesc`, `FrameDesc`.
- The thread contract for resource creation vs frame submission.
- The atomic-update-group contract that preserves the existing
  `CBuffDeferredModeStart/End` semantics for chunk rebuilds.

Input for this document is `RENDERER_CAPABILITY_AUDIT.md` (in this same
directory). Every row in the audit's Section 2 method inventory must map
unambiguously onto a field in one of the Desc structures defined here, or
be listed in Section 12 as dead and deleted. Section 12 of this document
is the validation matrix that confirms that.

This is a design spec, not production code. Exact struct packing, union
layouts, and memory footprints are Phase 2 concerns. The goal here is to
capture *what state exists*, *who owns it*, and *when it can be
submitted*, precisely enough that the Phase 2 header can be written
directly from this document.

### 0.1 Non-goals

- No file layout. The specs live in a single `IRenderPath.h` in Phase 2.
- No bgfx translation. Phase 7 backend mapping is not constrained here.
- No shading improvements. The shading model is the one the audit
  describes: fixed-function-equivalent directional lighting plus lightmap
  texture plus fog. No PBR, no shadow maps, no compute.
- No asset pipeline. `TextureDesc` accepts raw pixel data; PNG decoding
  lives in a separate `ImageLoader` after `LoadTextureData` is removed
  from the renderer interface.

## 1. Handle types

Every persistent renderer resource is addressed by a generational handle.
Plain `uint32_t` IDs are rejected: chunk rebuilds + stale handles is a
silent data corruption trap the audit's Section 7 makes explicit.

```cpp
struct MeshHandle {
    uint32_t index;
    uint32_t generation;
};

struct TextureHandle {
    uint32_t index;
    uint32_t generation;
};

struct MaterialHandle {
    uint32_t index;
    uint32_t generation;
};
```

Rules:

- `generation == 0` is reserved for the invalid handle. A default-
  constructed handle is invalid. Invalid handles compare equal.
- When a resource is destroyed and its slot recycled, the slot's
  generation is incremented. Any handle carrying the old generation is
  now stale; passing it to `update_*`, `destroy_*`, or referencing it
  from a `DrawCall` is a programming error and implementations must
  assert (debug) or silently drop the draw (release, documented).
- Handles are 8 bytes. 32-bit index gives 4B resources, 32-bit
  generation gives 4B recycle cycles per slot. Both are comfortably out
  of reach for this game.
- Handles are trivially copyable, trivially destructible, and POD. They
  can be stored in arrays, passed by value, and memcpy'd.
- Factory functions return `[[nodiscard]]` handles. Losing a handle is
  a leak warning at compile time, not a silent resource leak.

The invalid-handle constant:

```cpp
constexpr inline MeshHandle     kInvalidMesh    { 0, 0 };
constexpr inline TextureHandle  kInvalidTexture { 0, 0 };
constexpr inline MaterialHandle kInvalidMaterial{ 0, 0 };
```

## 2. Thread contract

One paragraph, deliberate, not up for interpretation.

Resource-creation methods (`create_mesh`, `update_mesh`, `destroy_mesh`,
`create_texture`, `update_texture`, `destroy_texture`, `create_material`,
`update_material`, `destroy_material`, plus the transient-vertex-buffer
allocator in Section 5.2) are *thread-transparent*. Callers may invoke
them from any thread without holding external locks. Returned handles
are valid immediately and usable in subsequent `render_frame` calls
(and in subsequent resource updates). The implementation is free to
serve these calls using an internal mutex, a lock-free command queue, or
by marshalling the underlying GPU work to its owning thread; none of
that is visible to the caller. This contract must preserve the existing
worker-thread chunk rebuild pipeline from audit Section 7: chunk
workers call `create_mesh` / `update_mesh` / `destroy_mesh` from the
rebuild thread, and the main thread replays them via draw-list entries
without coordination.

Frame submission (`render_frame`, `resize`) is *main-thread only*.
Implementations assert this in debug builds via a stored main-thread ID
captured at construction. Release builds document the contract but do
not enforce it (same as the current code).

The factory function (`make_legacy_gl_render_path`,
`make_bgfx_render_path`, etc.) is called once at startup on the main
thread. The returned `std::unique_ptr<IRenderPath>` is main-thread-
owned; the destructor runs on the main thread. Resource-creation
thread-transparency begins after construction completes and ends before
destruction starts.

### 2.1 Atomic update groups

The existing `CBuffDeferredModeStart` / `CBuffDeferredModeEnd` pair
(audit Section 7.1) exists so that a cluster of adjacent near-chunk
rebuilds can be committed to the GPU atomically, preventing visible
holes while one chunk is mid-rebuild. This semantics must survive.

Exposed as an RAII scope:

```cpp
struct ScopedResourceBatch {
    ScopedResourceBatch(IRenderPath& path);
    ~ScopedResourceBatch();  // commits all resource changes made during
                             // the scope as a single atomic group
    ScopedResourceBatch(const ScopedResourceBatch&) = delete;
    ScopedResourceBatch& operator=(const ScopedResourceBatch&) = delete;
    ScopedResourceBatch(ScopedResourceBatch&&) = default;
    ScopedResourceBatch& operator=(ScopedResourceBatch&&) = default;
};
```

Usage (main-thread near-chunk rebuild, preserves current behaviour):

```cpp
if (veryNearCount > 0) {
    ScopedResourceBatch atomic_group = path->open_atomic_resource_batch();
    for (auto& chunk : near_chunks)
        chunk.rebuild();  // create_mesh / update_mesh calls inside here
    // destructor fires -> all changes become visible atomically
}
```

Scopes may nest; only the outermost commit is visible externally.
Scopes are thread-local: opening one on thread A does not affect calls
on thread B. If cross-thread atomicity is ever needed, Phase 7+ can add
a named group primitive. The audit's worker-thread rebuild path does
not need this because worker-thread rebuilds are already independent.

### 2.2 Static tier seal

`PlatformRenderer.CBuffLockStaticCreations()` at `Minecraft.cpp:393`
marks the point where the initial set of static resources has been
loaded and any future allocations should come from a different pool.
Preserved as:

```cpp
void seal_static_resource_tier();
```

After this call, new `create_*` requests go into a "dynamic" tier that
the backend may allocate differently (separate arena, separate mip
pool, whatever). The distinction is advisory: the caller does not need
to track which tier a handle belongs to. Only one seal is allowed per
path instance; subsequent calls are no-ops.

## 3. `TextureDesc` and friends

### 3.1 `TextureDesc`

```cpp
enum class TextureFormat : uint8_t {
    rgba8_unorm,   // the only format used by 4jcraft today
    // future: rgba16f for HDR sky/lightmap, depth32f for shadows
};

enum class TextureFilter : uint8_t {
    nearest,
    linear,
    mipmap_nearest,
    mipmap_linear,
};

enum class TextureWrap : uint8_t {
    clamp_to_edge,
    repeat,
};

enum class TextureUsage : uint8_t {
    static_resource,     // uploaded once, read many
    dynamic_stream,      // updated every frame (lightmap, sky tint)
    render_target,       // target of a later pass (out of scope today)
    readback,            // CPU-readable (screenshot/thumbnail)
};

struct TextureDesc {
    uint32_t width;
    uint32_t height;
    uint8_t  mip_levels;      // 1 for non-mipped; >1 implies the initial
                              // data contains levels 0..mip_levels-1
    TextureFormat format;
    TextureUsage  usage;
    TextureFilter min_filter;
    TextureFilter mag_filter;
    TextureWrap   wrap_s;
    TextureWrap   wrap_t;
    std::span<const std::byte> initial_data;  // may be empty for usage=dynamic_stream
                                              // or usage=render_target
    const char*   debug_name;  // RenderDoc label, may be nullptr
};
```

Ownership: `initial_data` is borrowed for the duration of the call. The
implementation copies what it needs. The caller may free the backing
buffer as soon as `create_texture` returns. `debug_name` must point to
a string with lifetime at least until `destroy_texture`; string literals
are fine.

### 3.2 `TextureRegion` for partial updates

```cpp
struct TextureRegion {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint8_t  mip_level;
    std::span<const std::byte> data;  // exactly width*height*format_bytes
};
```

Used by `update_texture(TextureHandle, const TextureRegion&)`. Covers
both full-mip upload and sub-region updates. Handles the audit's
`TextureData` (full upload) + `TextureDataUpdate` (region update) call
sites uniformly. The caller provides tightly-packed pixel data in the
texture's format; the backend is free to convert on upload but must
not assume any particular tiling or padding.

### 3.3 Readback

```cpp
struct TextureReadback {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    std::span<std::byte> out;  // must be width*height*format_bytes
};

void read_framebuffer(const TextureReadback& req);
```

Read-from-current-frame. Blocking, main-thread only. Replaces the
audit's internal `ReadPixels` call at `GLRenderer.cpp:1493` and the
`glReadPixels_4J` wrapper. Screenshot and thumbnail capture are
downstream users of this; `CaptureThumbnail` / `CaptureScreen` from
the old interface are not part of `IRenderPath` at all (they belong in
a downstream helper that calls `read_framebuffer` and encodes).

## 4. Vertex layouts

Three layouts are observed by the audit:

```cpp
enum class VertexLayout : uint8_t {
    chunk_compact,    // VERTEX_TYPE_COMPRESSED - packed chunk vertex
    world_standard,   // PF3_TF2_CB4_NB4_XW1 - standard non-chunk geometry
    world_texgen,     // PF3_TF2_CB4_NB4_XW1_TEXGEN - standard + texgen slot
};
```

The dead enum values `VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT` (pre-lit, no
callers) and `PIXEL_SHADER_TYPE_FORCELOD` (as a *layout* selector, not
as a forced-LOD override) do not appear here.

Exact byte layouts are frozen in Phase 2 when the header is written,
because they must match the existing buffer contents bit-for-bit during
the `LegacyGLRenderPath` stage. The layouts are:

- `chunk_compact`: 16 bytes per vertex (TBC in Phase 2), position as
  fixed-point ints relative to chunk origin, packed normal, packed UVs,
  one byte-color. Current use: chunk geometry built by
  `Tesselator::useCompactVertices(true)`.
- `world_standard`: 32 bytes per vertex. Float3 position + Float2
  texcoord + Byte4 color + Byte4 normal + Word1 lightmap-uv or extra.
  Current use: everything non-chunk (entities, particles, GUI, HUD).
- `world_texgen`: 32 bytes per vertex, same shape as `world_standard`
  but the extra word is interpreted as texgen data by the projected-
  texture shader. Current use: `Tesselator.cpp:130` projected-texture
  path (selection outline, shadow, decals).

The primitive type is a separate field:

```cpp
enum class PrimitiveType : uint8_t {
    triangle_list,
    triangle_strip,
    triangle_fan,
    line_list,
    line_strip,
    // quad_list deliberately omitted - TRIANGLE_MODE unrolls quads at
    // submission time; confirm quad_list is unreachable in Phase 2.
};
```

## 5. Mesh resources

Two kinds of vertex data exist in the audit's usage patterns: persistent
(chunks, cached UI command buffers) and transient (per-frame Tesselator
output from entities, particles, GUI immediate). They have different
lifetimes and different submission costs, so they get two primitives.

### 5.1 `MeshDesc` (persistent)

```cpp
struct MeshBounds {
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
};

struct MeshDesc {
    VertexLayout  layout;
    PrimitiveType primitive;
    uint32_t      vertex_count;
    std::span<const std::byte> vertex_data;   // vertex_count * sizeof(layout)

    // Optional index buffer. Empty span = non-indexed draw.
    std::span<const uint32_t> indices;

    // Optional precomputed AABB in mesh-local space. All-zero = compute
    // from vertex_data on upload (slower path).
    MeshBounds    bounds;

    // Usage hint. Does not affect semantics, only memory tiering.
    enum class Usage : uint8_t {
        static_lifetime,   // chunk geometry, cached UI buffers
        streaming,         // rebuilt every few frames (not used today)
    } usage;

    const char* debug_name;  // may be nullptr
};

[[nodiscard]] MeshHandle create_mesh(const MeshDesc& desc);
               void       update_mesh(MeshHandle, const MeshDesc& desc);
               void       destroy_mesh(MeshHandle);
```

Ownership: `vertex_data` and `indices` are borrowed; the implementation
copies what it needs before returning. Callers can free their source
buffers immediately. The `debug_name` string must outlive the mesh.

`update_mesh` replaces all contents of an existing handle. It is a
convenience over destroy+create that preserves the handle. Typical
caller: chunk rebuild - a chunk moves out of view, its handle is
destroyed; a new chunk moves in, gets a new handle. In-place rebuild
(same chunk, different geometry) uses `update_mesh`.

This is the home for the audit's `CBuffCreate` / `CBuffDelete` /
`CBuffDeleteAll` / `CBuffClear` call sites (from chunk rebuild and
LevelRenderer teardown). `CBuffClear` maps to `update_mesh` with an
empty `vertex_data` span (a valid empty mesh; subsequent draws
referencing the handle render nothing).

### 5.2 `TransientVertexBuffer` (per-frame)

For Tesselator output, entity draws, particle draws, GUI immediate
quads, and anything else that uploads fresh vertex data every frame.
Modelled on bgfx's transient-buffer idiom.

```cpp
struct TransientVertexBuffer {
    uint32_t      frame_index;   // which frame this alloc belongs to
    uint32_t      offset;        // opaque handle into the frame arena
    uint32_t      vertex_count;
    VertexLayout  layout;
    PrimitiveType primitive;
};

// Allocates vertex_count vertices in the layout from the current
// frame's transient arena. The returned span points into the arena;
// caller fills it with vertex data. Span and the returned struct are
// only valid until the next `render_frame` call commits the frame.
[[nodiscard]] std::pair<TransientVertexBuffer, std::span<std::byte>>
    alloc_transient_vertices(
        uint32_t      vertex_count,
        VertexLayout  layout,
        PrimitiveType primitive);
```

Thread contract: allocatable from any thread (thread-transparent), but
the returned `TransientVertexBuffer` handle must be submitted via a
`DrawCall` in the same frame that allocated it. Cross-frame use is a
programming error and an assert.

No handle generation is needed because the handle is not stored across
frames; `frame_index` serves the equivalent stale-check role (mismatch
between handle frame and current frame is an assert).

The transient arena is owned by the render path, sized at construction
time (default: 16 MB, configurable), and reset at the start of each
`render_frame`. If an allocation exceeds the remaining space, the
allocator returns an invalid handle and logs; the caller must gracefully
drop the draw.

## 6. Materials

### 6.1 `MaterialDesc`

Material captures the parts of render state that are stable across
many draws of the same class (terrain, entities, HUD, etc). Per-draw
variance goes on `DrawCall` (Section 7), per-frame environment goes
on `ViewDesc` (Section 8).

```cpp
enum class ShaderPath : uint8_t {
    standard,            // PIXEL_SHADER_TYPE_STANDARD
    projected_texture,   // PIXEL_SHADER_TYPE_PROJECTION
};

enum class BlendMode : uint8_t {
    opaque,              // no blending
    alpha,               // src_alpha / one_minus_src_alpha
    additive,            // src_alpha / one
    multiply,            // dst_color / zero (rare, shadow/darken)
    premultiplied,       // one / one_minus_src_alpha
    custom,              // use explicit src/dst fields below
};

enum class BlendFactor : uint8_t {
    zero, one,
    src_color, one_minus_src_color,
    src_alpha, one_minus_src_alpha,
    dst_color, one_minus_dst_color,
    dst_alpha, one_minus_dst_alpha,
    constant_alpha, one_minus_constant_alpha,
};

enum class AlphaTest : uint8_t {
    off,
    greater,             // GL_GREATER
    greater_equal,       // GL_GEQUAL
    equal,               // GL_EQUAL
};

enum class DepthTest : uint8_t {
    off,
    less,
    less_equal,          // GL_LEQUAL (default in the current code)
    equal,
    greater,
    greater_equal,
    always,
};

enum class CullMode : uint8_t {
    none,
    back_ccw,            // default: back-face culling with CCW front
    back_cw,             // for negatively-scaled transforms
    front,               // rare, used for inside-of-sphere effects
};

struct MaterialDesc {
    ShaderPath    shader;
    BlendMode     blend;
    BlendFactor   blend_src_custom;  // only read if blend == custom
    BlendFactor   blend_dst_custom;  // only read if blend == custom
    AlphaTest     alpha_test;
    float         alpha_ref;         // only read if alpha_test != off
    DepthTest     depth_test;
    bool          depth_write;
    CullMode      cull;
    bool          lit;               // true = lighting env applied,
                                     //   false = unlit (sky, GUI, debug)
    bool          textured;          // false = use vertex color only
                                     //   (wireframe overlays, debug bounds)
    bool          fog_enabled;       // false = ignore view fog env

    // Static texture bindings. Slots are: 0 = diffuse, 1 = lightmap,
    // 2-3 = material-specific (decal, detail, normal in future). Dynamic
    // or per-draw texture overrides go on DrawCall, not here.
    TextureHandle texture_slots[4];

    const char*   debug_name;
};

[[nodiscard]] MaterialHandle create_material(const MaterialDesc& desc);
              void            update_material(MaterialHandle, const MaterialDesc& desc);
              void            destroy_material(MaterialHandle);
```

### 6.2 State ownership matrix

The master plan requires this. Every piece of render state lives in
exactly one of four places: `MaterialDesc`, `DrawCall`, `ViewDesc`, or
host-state (window, framebuffer). Duplicating state across two places
is a bug.

| State | Owner | Audit origin |
|---|---|---|
| Shader path (standard / projected) | `MaterialDesc::shader` | Section 3.2 |
| Vertex layout | `MeshDesc::layout` / `TransientVertexBuffer::layout` | Section 3.1 |
| Primitive type | `MeshDesc::primitive` / `TransientVertexBuffer::primitive` | Section 3.1 |
| Blend mode + factors | `MaterialDesc::blend{,_src,_dst}` | Section 2 (StateSetBlend*) |
| Blend constant factor | `DrawCall::blend_constant_factor` | Section 2 (StateSetBlendFactor, Gui.cpp) |
| Alpha test + ref | `MaterialDesc::alpha_test`, `alpha_ref` | Section 2 (StateSetAlpha*) |
| Depth test / func / write | `MaterialDesc::depth_test`, `depth_write` | Section 2 (StateSetDepth*) |
| Depth bias (polygon offset) | `DrawCall::depth_bias` | Section 2 (StateSetDepthSlopeAndBias) |
| Cull mode / winding | `MaterialDesc::cull` | Section 2 (StateSetFaceCull*) |
| Line width | `DrawCall::line_width` | Section 2 (StateSetLineWidth, LevelRenderer debug bounds) |
| Lighting enable (per-draw) | `MaterialDesc::lit` + can be overridden at `DrawCall::lit_override` for edge cases | Section 9.2 |
| Lit environment (directions, colors, ambient) | `ViewDesc::lighting` | Section 9.2 |
| Lightmap texture | `ViewDesc::lighting.lightmap` | Section 4.5 |
| Fog enable (per-draw) | `MaterialDesc::fog_enabled` | Section 9.1 |
| Fog environment (color, mode, start, end, density) | `ViewDesc::fog_profiles[]` + `DrawCall::fog_profile_idx` | Section 9.1 |
| Texture bindings (static per-material) | `MaterialDesc::texture_slots[]` | Section 4.3 |
| Texture binding (per-draw override) | `DrawCall::texture_override` | Section 4.3 (HorseRenderer mob texture) |
| Texture enable | `MaterialDesc::textured` | Section 2 (StateSetTextureEnable) |
| Active texture unit selection | disappears - multi-texture is explicit per-slot | Section 4.3 |
| Stencil op (ref / mask / write mask / func) | `DrawCall::stencil` (optional) | Section 11.3 (UIScene_SkinSelectMenu) |
| Forced LOD bias | `DrawCall::forced_lod` | Section 2 (StateSetForceLOD) |
| Tint colour | `DrawCall::tint_color` | Section 2 (StateSetColour, via glColor4f macro) |
| Transform | `DrawCall::transform` | Section 8 (matrix stack -> per-draw mat4) |
| Chunk-offset per-draw vec3 | `ChunkDrawCall::chunk_offset` (specialisation of DrawCall) | Section 10.1 (SetChunkOffset) |
| Projection matrix | `ViewDesc::projection` | Section 8.1 |
| View matrix | `ViewDesc::view` | Section 8.1 |
| Viewport layout (9-value) | `ViewDesc::viewport_layout` | Section 6.1 |
| Scissor from viewport | `ViewDesc::scissor_from_viewport` | Section 6.3 |
| Color mask (anaglyph channel mask) | `ViewDesc::color_mask` | Section 2 (StateSetWriteEnable) |
| Clear flags / values | `ViewDesc::clear` | Section 2 (Clear, SetClearColour, glClearColor in GameRenderer) |
| Framebuffer size | host-owned, surfaced via `FrameDesc::framebuffer` | Section 5 (GetFramebufferSize) |
| `IsHiDef` / `IsWidescreen` | host-owned, surfaced via `FrameDesc::framebuffer.{resolution_tier, aspect_hint}` | Section 2 |

The fields `MaterialDesc::lit`, `MaterialDesc::textured`, and
`MaterialDesc::fog_enabled` are per-material booleans that let a draw
opt out of the view's environment. `DrawCall::lit_override` exists only
for the debug bounding box / wireframe case where the same material is
sometimes used lit and sometimes unlit; Phase 3 can determine whether
the two cases actually need distinct materials instead. Prefer two
materials.

## 7. `DrawCall`

```cpp
enum class VertexSource : uint8_t { mesh, transient };

// Optional stencil operation for the rare UI draw that uses stencil.
struct StencilOp {
    DepthTest func;        // reuses the compare enum, GL_EQUAL etc
    uint8_t ref;
    uint8_t func_mask;
    uint8_t write_mask;
};

struct DrawCall {
    VertexSource     source;
    union {
        MeshHandle            mesh;
        TransientVertexBuffer transient;
    };

    MaterialHandle   material;

    // World/model transform. For chunk draws in a `ChunkDrawCall`
    // subtype (see below), this is replaced by `chunk_offset` and the
    // transform slot is ignored. For non-chunk draws this is always a
    // full 4x4.
    float            transform[16];

    // Per-draw tint. Multiplied with vertex colour in the shader.
    // Default (1,1,1,1) = no tint.
    float            tint_color[4];

    // Per-draw texture override for slot 0 (diffuse). Set to
    // kInvalidTexture to use the material's slot 0. Other slots are
    // always read from the material.
    TextureHandle    texture_override;

    // Fog profile index, 0..3. Index 0 is the view's default fog. The
    // per-pass fog behaviour from `GameRenderer::setupFog(i, ...)`
    // (sky/world/item-in-hand) uses indices 1 and 2.
    uint8_t          fog_profile_idx;

    // Forced LOD bias. 0 = no forcing, -1 = default from audit.
    int8_t           forced_lod;

    // Polygon offset for z-fighting prevention (debug overlays, shadow
    // projection). Slope and units. {0,0} = disabled.
    float            depth_slope;
    float            depth_bias;

    // Line width for PRIMITIVE_TYPE_LINE_* primitives. Ignored
    // otherwise. 0 = default 1px.
    float            line_width;

    // Constant blend factor for MaterialDesc::blend where a factor
    // depends on a constant colour (e.g., GL_CONSTANT_ALPHA for the
    // flash effect in Gui.cpp).
    uint32_t         blend_constant_factor;

    // Optional stencil op. Nullptr = no stencil state change.
    const StencilOp* stencil;

    // Debug-only lit override: forces lighting off even if
    // MaterialDesc::lit is true. Used by debug wireframe passes that
    // want to share the material of the thing they're outlining.
    // Normal draws leave this false. Phase 3 should try to delete it.
    bool             lit_override_off;
};

struct ChunkDrawCall {
    VertexSource     source;          // always mesh
    MeshHandle       mesh;
    MaterialHandle   material;

    // Chunk origin in world space. Replaces the per-draw transform
    // because chunk vertices are already baked in chunk-local space
    // and the shader adds this offset directly.
    float            chunk_offset[3];

    uint8_t          fog_profile_idx;
    int8_t           forced_lod;
};
```

`ChunkDrawCall` is a memory-footprint specialisation of the world
draw path: a full `mat4` per chunk is wasteful when the only
per-chunk data is a vec3 translation. The audit's `SetChunkOffset`
call at `LevelRenderer.cpp:863-872` is exactly this optimisation,
already applied to the legacy backend. Phase 2 Keeps it.

Draw list bins (Section 8.4) are homogeneous: one bin holds only
`DrawCall`, another holds only `ChunkDrawCall`. There is no heterogeneous
draw list.

## 8. `ViewDesc`

A view is one camera rendering one scene into one viewport. Frames
have a list of views. The audit's split-screen entry point
(`Minecraft.cpp:1634`) produces one view per player plus optional
frontend/quadrant-clear views.

```cpp
enum class ViewportLayout : uint8_t {
    fullscreen,
    split_top,    split_bottom,      // 2-player horizontal split
    split_left,   split_right,       // 2-player vertical split
    quadrant_top_left,   quadrant_top_right,
    quadrant_bottom_left, quadrant_bottom_right,   // 3-4 player quadrants
};
```

All nine values are live (audit Section 6.1). The enum intentionally
matches the layout of the legacy `IPlatformRenderer::eViewportType`
enum so the migration is a direct value rename, not a logic change.

### 8.1 Camera and clipping

```cpp
struct ViewCamera {
    float projection[16];  // CPU-built projection matrix (perspective
                           // or orthographic). Replaces the fixed-
                           // function glMatrixMode(GL_PROJECTION) +
                           // glLoadIdentity + MatrixPerspective sequence.
    float view[16];        // CPU-built view matrix. Replaces
                           // glMatrixMode(GL_MODELVIEW) + glLoadIdentity
                           // + bobHurt/bobView/moveCameraToPlayer.
};
```

The audit's Section 5.1 notes that `Camera::prepare()` already produces
these matrices every frame; migrating `Frustum::calculateFrustum` to
read from `Camera` instead of `MatrixGet` is the same data change.

### 8.2 Clear, color mask, scissor

```cpp
enum ClearFlags : uint8_t {
    CLEAR_NONE    = 0,
    CLEAR_COLOR   = 1 << 0,
    CLEAR_DEPTH   = 1 << 1,
    CLEAR_STENCIL = 1 << 2,
};

struct ViewClear {
    uint8_t flags;         // bitfield of ClearFlags
    float   color[4];      // used iff CLEAR_COLOR set
    float   depth;         // used iff CLEAR_DEPTH set, default 1.0
    uint8_t stencil;       // used iff CLEAR_STENCIL set, default 0
};

struct ViewColorMask {
    bool r, g, b, a;       // default true-true-true-false (matches
                           // the audit's `StateSetWriteEnable(true,
                           // true, true, false)` at GameRenderer.cpp:1517)
};
```

The anaglyph rendering path from `GameRenderer.cpp:1264-1266` uses the
color mask to draw each eye into a different channel. Per-view
`color_mask` captures this: left eye view has `{true, false, false,
false}`, right eye view has `{false, true, true, false}`. No
per-draw color mask is needed.

### 8.3 Fog and lighting environment

```cpp
enum class FogMode : uint8_t {
    disabled,
    linear,            // GL_LINEAR: start, end
    exponential,       // GL_EXP: density
    exponential_sq,    // GL_EXP2: density (audit notes only EXP is used,
                       //   EXP2 is placeholder for near-future parity)
};

struct FogProfile {
    FogMode mode;
    float   color[3];   // RGB
    float   start;      // used when mode == linear
    float   end;        // used when mode == linear
    float   density;    // used when mode == exponential / exponential_sq
};

struct DirectionalLight {
    bool    enabled;
    float   direction[3];  // normalized, view or world space (Phase 2)
    float   color[3];
};

struct LightingEnv {
    float              ambient_color[3];
    DirectionalLight   directional[2];   // audit Section 9.2: two lights
    TextureHandle      lightmap;         // audit Section 4.5: 16x16 dynamic
};

struct ViewDesc {
    ViewportLayout  viewport_layout;
    ViewCamera      camera;
    ViewClear       clear;
    ViewColorMask   color_mask;
    bool            scissor_from_viewport;  // audit Section 6.3

    // Up to 4 fog profiles. Index 0 is the view's default; other
    // indices are referenced by DrawCall::fog_profile_idx. The
    // per-pass fog model in audit Section 9.1 uses three profiles
    // in practice (sky, world, item-in-hand). Four leaves slack.
    FogProfile      fog_profiles[4];
    uint8_t         fog_profile_count;    // 1..4

    LightingEnv     lighting;

    // Draw list bins. Order within each bin is submission order.
    // Between bins, the path renders in the order: opaque -> alpha_test
    // -> transparent -> debug_overlay. Phase 2 can reorder if bgfx view
    // bins demand it.
    std::span<const ChunkDrawCall> chunk_opaque;
    std::span<const ChunkDrawCall> chunk_alpha_test;
    std::span<const ChunkDrawCall> chunk_transparent;
    std::span<const DrawCall>      world_opaque;
    std::span<const DrawCall>      world_alpha_test;
    std::span<const DrawCall>      world_transparent;   // sorted back-to-front
    std::span<const DrawCall>      debug_overlay;       // depth-tested
                                                        //   wireframes, bounds
};
```

Four fog profiles covers the audit's observed use. Chunk bins and
world bins are separated because chunks are overwhelmingly common
and benefit from the `ChunkDrawCall` specialisation; mixing them would
force every draw into the larger struct.

Debug overlay is a world-space bin that is depth-tested. GUI / HUD /
cross-hair overlay is screen-space and lives on `FrameDesc`, not here.
This matches the feedback the audit already called out.

## 9. `FrameDesc`

```cpp
struct FrameFramebuffer {
    uint32_t width;
    uint32_t height;
    float    aspect;         // width/height, convenience
    bool     is_widescreen;  // was IsWidescreen()
    bool     is_hi_def;      // was IsHiDef()
};

struct FrameDesc {
    FrameFramebuffer       framebuffer;
    double                 current_time_seconds;
    float                  delta_time_seconds;
    uint64_t               frame_index;

    // List of views. Always non-empty in a game frame; the frontend
    // path submits a single fullscreen view.
    std::span<const ViewDesc> views;

    // Screen-space UI overlay drawn after all views, no depth test,
    // projection is implicit (identity ortho over the framebuffer).
    // Populated by Gui.cpp, Screen.cpp, HUD components, UIScene.
    std::span<const DrawCall> ui_overlay;
};

void render_frame(const FrameDesc& frame);
```

`ui_overlay` is the third of the three submission categories from the
master plan: transient screen-space overlay. Debug world-space lines
are not here; they live on the relevant `ViewDesc::debug_overlay`
bin so they get the correct projection and depth test for their view.

The caller holds the backing storage for all the spans during the
duration of `render_frame`. The implementation must have finished
consuming them by the time `render_frame` returns. No deferred,
cross-frame span access. This keeps the ownership model simple and
matches the legacy path's immediate-mode translation step.

## 10. Resource registry back-pressure

The audit's `CBuffSize(-1)` call at `LevelRenderer.cpp:1750` reads the
total GPU memory footprint of all command buffers as a back-pressure
signal for chunk rebuild scheduling. Preserved as:

```cpp
struct ResourceFootprint {
    uint64_t mesh_bytes;
    uint64_t texture_bytes;
    uint64_t total_bytes;
};

[[nodiscard]] ResourceFootprint query_resource_footprint() const;
```

`LevelRenderer` uses this to decide `onlyRebuild = (total_bytes >=
MAX_COMMANDBUFFER_ALLOCATIONS)`. Calling this every frame is fine; it
reads a single atomic counter on the implementation side.

Budgets and soft limits are advisory. The backend does not enforce
them; the caller decides how to react.

## 11. What happens at `render_frame`

Pseudocode for the legacy and bgfx paths alike:

```
render_frame(FrameDesc frame):
    begin backend frame (clear transient arena, reset frame_index)
    for each ViewDesc v in frame.views:
        set viewport from v.viewport_layout
        apply v.clear
        apply v.color_mask
        apply v.scissor_from_viewport
        upload v.camera to shader uniforms
        upload v.lighting to shader uniforms
        for each fog profile up to v.fog_profile_count:
            upload to slot i
        for each bin in [chunk_opaque, chunk_alpha_test,
                         chunk_transparent, world_opaque,
                         world_alpha_test, world_transparent,
                         debug_overlay]:
            for each draw in bin:
                apply draw.material
                apply draw per-draw state (fog_profile_idx, forced_lod,
                     depth_bias, line_width, tint, stencil)
                upload draw.transform (or chunk_offset)
                submit draw using draw.mesh or draw.transient
    set viewport to fullscreen
    disable depth test
    for each draw in frame.ui_overlay:
        same as above with identity camera
    end backend frame
```

The legacy `LegacyGLRenderPath` implements this by translating every
step into the existing `PlatformRenderer.*` calls. The bgfx path
implements it against bgfx views (one bgfx view per `ViewDesc` plus
one for the UI overlay). Neither of those implementations are part of
Phase 1; they are Phase 2 and Phase 7 respectively.

## 12. Validation against audit Section 2

Every row in `RENDERER_CAPABILITY_AUDIT.md` Section 2 maps to something
in this document or is dead. Grouped by audit disposition.

### 12.1 Reshape into Desc types

| Audit row | This doc's home |
|---|---|
| `StartFrame` / `Present` / `Clear` / `SetClearColour` | Frame enter/exit is `render_frame`. Clear is `ViewDesc::clear`. |
| `MatrixMode` / `MatrixSetIdentity` / `MatrixTranslate` / `MatrixRotate` / `MatrixScale` / `MatrixMult` / `MatrixPush` / `MatrixPop` | `ViewDesc::camera.view` / `ViewDesc::camera.projection` + `DrawCall::transform`. Stack disappears. |
| `MatrixPerspective` / `MatrixOrthogonal` | `ViewDesc::camera.projection` (CPU-built). |
| `MatrixGet` (external) | Deleted. `Frustum::calculateFrustum` reads matrices from `Camera` as described in audit Section 5.1. |
| `Set_matrixDirty` | Deleted. Shadow state is gone. |
| `DrawVertices` | `DrawCall` + `MeshDesc` / `TransientVertexBuffer` (Sections 5, 7). |
| `CBuffCreate` / `CBuffDelete` / `CBuffDeleteAll` | `create_mesh` / `destroy_mesh` / registry teardown. |
| `CBuffStart` / `CBuffEnd` | `create_mesh` / `update_mesh` entry points; vertex data goes directly in `MeshDesc::vertex_data`. |
| `CBuffClear` | `update_mesh` with empty `vertex_data`. |
| `CBuffCall` | Draw list entry in `ViewDesc::chunk_*` bins with a `MeshHandle`. |
| `CBuffSize` | `query_resource_footprint()`. |
| `CBuffDeferredModeStart` / `End` | `ScopedResourceBatch` (Section 2.1). |
| `CBuffLockStaticCreations` | `seal_static_resource_tier()` (Section 2.2). |
| `CBuffTick` | Internal backend tick, not on the public surface. |
| `TextureCreate` / `TextureFree` | `create_texture` / `destroy_texture`. |
| `TextureBind` / `TextureBindVertex` / `StateSetActiveTexture` | `MaterialDesc::texture_slots[]` + `DrawCall::texture_override`. |
| `TextureData` / `TextureDataUpdate` | `TextureDesc::initial_data` + `update_texture(TextureRegion)`. |
| `TextureSetTextureLevels` | `TextureDesc::mip_levels`. |
| `TextureGetTextureLevels` | Deleted. Callers (Texture.cpp:599) already set the mip count themselves via `TextureSetTextureLevels` on the preceding line; migrate to a caller-owned `mip_levels` local. |
| `TextureSetParam` | `TextureDesc::min_filter` / `mag_filter` / `wrap_s` / `wrap_t`. |
| `LoadTextureData` (filename + memory) | Moved out of the renderer interface into `ImageLoader`. `TextureDesc::initial_data` is raw pixel bytes. |
| `ReadPixels` | `read_framebuffer(TextureReadback)`. |
| `StateSetColour` | `DrawCall::tint_color`. |
| `StateSetDepthMask` | `MaterialDesc::depth_write`. |
| `StateSetBlendEnable` / `StateSetBlendFunc` | `MaterialDesc::blend` (+ custom factors). |
| `StateSetBlendFactor` | `DrawCall::blend_constant_factor`. |
| `StateSetAlphaFunc` / `StateSetAlphaTestEnable` | `MaterialDesc::alpha_test` + `alpha_ref`. |
| `StateSetDepthFunc` / `StateSetDepthTestEnable` | `MaterialDesc::depth_test`. |
| `StateSetFaceCull` / `StateSetFaceCullCW` | `MaterialDesc::cull`. |
| `StateSetLineWidth` | `DrawCall::line_width`. |
| `StateSetWriteEnable` | `ViewDesc::color_mask`. |
| `StateSetDepthSlopeAndBias` | `DrawCall::depth_slope` / `depth_bias`. |
| `StateSetFogEnable` / `FogMode` / `FogNearDistance` / `FogFarDistance` / `FogDensity` / `FogColour` | `ViewDesc::fog_profiles[]` + `DrawCall::fog_profile_idx` + `MaterialDesc::fog_enabled`. |
| `StateSetLightingEnable` | `MaterialDesc::lit` + `DrawCall::lit_override_off`. |
| `StateSetVertexTextureUV` | Per-vertex lightmap UV, already in vertex layout (`world_standard` extra word). Delete. |
| `StateSetLightColour` / `LightAmbientColour` / `LightDirection` / `LightEnable` | `ViewDesc::lighting`. |
| `StateSetViewport` | `ViewDesc::viewport_layout`. |
| `StateSetEnableViewportClipPlanes` | `ViewDesc::scissor_from_viewport`. |
| `StateSetTexGenCol` | Folded into the projected-texture shader path; data on `DrawCall` or `MaterialDesc` (Phase 2 decides). |
| `StateSetStencil` | `DrawCall::stencil`. |
| `StateSetForceLOD` | `DrawCall::forced_lod`. |
| `StateSetTextureEnable` | `MaterialDesc::textured`. |
| `SetChunkOffset` | `ChunkDrawCall::chunk_offset`. |
| `GetFramebufferSize` / `IsHiDef` / `IsWidescreen` | `FrameDesc::framebuffer`. |
| `BeginEvent` / `EndEvent` | Not on `FrameDesc`. Backend debug markers stay as direct methods (`path.push_debug_event` / `pop_debug_event`) called from game code that wants RenderDoc labels. |

### 12.2 Keep as lifecycle / host methods (not on `FrameDesc`)

| Audit row | Home |
|---|---|
| `Initialise` / `InitialiseContext` / `Shutdown` | Factory construction / destruction of the `IRenderPath`. |
| `Tick` | Host-layer tick, called from main loop. Stays on the path as a host hook if the backend needs it; otherwise deleted. |
| `Suspend` / `Suspended` / `Resume` | Backend lifecycle hooks for platforms with app-suspend semantics. Stay as direct methods. |
| `SetWindowSize` / `SetFullscreen` / `ShouldClose` / `Close` / `UpdateGamma` | Host window methods, NOT on `IRenderPath`. Move to a separate `IWindow` / `IWindowHost` in Phase 2. |
| `CBuffLockStaticCreations` | `seal_static_resource_tier()` on the path. |

### 12.3 Confirmed dead (no home needed)

Matches audit Section 12:

- `VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT`
- `PIXEL_SHADER_TYPE_FORCELOD` as an enum value
- `TextureGetStats`
- `TextureGetTexture`
- `SaveTextureData`
- `SaveTextureDataToMemory`
- `DoScreenGrabOnNextPresent`
- `TextureDynamicUpdateStart` / `TextureDynamicUpdateEnd`

### 12.4 Probably dead - verify before Phase 2

Matches audit Section 12. If confirmed dead they do not appear in the
new interface at all; if still used, they get explicit homes.

- `BeginConditionalSurvey` / `EndConditionalSurvey` / `BeginConditionalRendering` / `EndConditionalRendering` - occlusion query path. If the `occlusion_culling` meson option includes a hardware-query mode in active use, these get exposed as a small query API on `IRenderPath`. Otherwise dead.
- `CaptureThumbnail` / `CaptureScreen` - if live, move to a helper that calls `read_framebuffer`. If dead, delete.

### 12.5 Gaps

On a full sweep of the audit's Section 2 against the matrix above, no
rows are unplaced. The spec covers the full inventory.

## 13. Open decisions for Phase 2

These are small enough not to block the header write but must be
decided before the first `IRenderPath.h` commit:

1. **Exact byte layout for the three vertex formats.** Phase 2 must
   match the existing buffer contents bit-for-bit during
   `LegacyGLRenderPath`. Requires reading `GLRenderer.cpp`'s
   `DrawVertices` implementation to pin down byte offsets, stride, and
   attribute types. Not doing this in Phase 1 because the audit's
   scope was external callers, not the backend implementation.
2. **Transient arena size.** 16 MB default proposed above. Measure
   against a busy frame (many entities, many particles, UI, HUD) to
   confirm headroom.
3. **Whether `DrawCall::transform` is inline (64 bytes) or indexed
   into a side arena.** Inline is simpler, indexed is cache-friendlier
   for large lists. Keep inline in Phase 2; profile in Phase 6 if
   needed.
4. **Whether `ChunkDrawCall` is a separate type or a flag on
   `DrawCall`.** Separate type proposed here. Phase 2 may merge if the
   discriminator cost is negligible.
5. **Where `StateSetTexGenCol` ends up.** Projected-texture shader path
   reads it. Likely a field on `MaterialDesc` since the data is per-
   material (the texgen plane doesn't change per draw), but the
   existing call sites need another grep to confirm.
6. **`IWindow` vs `IRenderPath` split.** Window methods (`SetWindowSize`,
   `Close`, `ShouldClose`, `UpdateGamma`) are clearly host-layer and
   not rendering, but the current code has them on `IPlatformRenderer`.
   Phase 2 decides whether to split them now or keep them on the
   legacy methods section until Phase 8 deletion.

None of these change the shape of the Descs; they are implementation
choices downstream.

## 14. Next step

Phase 1 closes here. Phase 2 begins by writing `IRenderPath.h` + the
`LegacyGLRenderPath.{h,cpp}` skeleton per the master plan. The header
is a direct mechanical transcription of Sections 1-10 of this document
plus the `[[deprecated]]` legacy section that covers any audit-Section-2
row still in transition at the time of writing.

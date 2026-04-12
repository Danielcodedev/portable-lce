# Renderer Capability Audit

## 1. Scope and purpose

Read-only inventory of every caller of `IPlatformRenderer` in this repository,
generated against branch `renderer-work` at commit `397ab0a6e` (identical to
`platform-refactor`). The goal is to name every real usage pattern so that the
future `IRenderPath` / `MeshDesc` / `MaterialDesc` / `ViewDesc` / `FrameDesc`
specs have an unambiguous source of truth. This document is Phase 0 of the
renderer refactor; no code changes are proposed here.

Interface header: `targets/platform/renderer/IPlatformRenderer.h` (208 lines,
one pure virtual class).

Forwarding header that the rest of the codebase includes:
`targets/platform/renderer/renderer.h`. Exposes the single global
`PlatformRenderer` macro that all call sites use.

GL implementation (the only backend today): `targets/platform/renderer/gl/`
(`GLRenderer.h`, `GLRenderer.cpp`, `gl_compat.h`, `render_stubs.cpp`).

### 1.1 The `gl_compat.h` macro layer is critical and easy to miss

A grep for `PlatformRenderer\.` finds ~140 direct call sites across the repo.
This understates the real migration surface by a large factor because
`targets/platform/renderer/gl/gl_compat.h` re-defines legacy `gl*` symbols as
macros that expand to `PlatformRenderer.*` calls. Every `glPushMatrix`,
`glPopMatrix`, `glMatrixMode`, `glLoadIdentity`, `glTranslatef`, `glRotatef`,
`glScalef`, `glColor4f`, `glAlphaFunc`, `glOrtho`, `glEnable(GL_FOG|
GL_LIGHTING|GL_ALPHA_TEST|GL_TEXTURE_2D|GL_BLEND|GL_CULL_FACE|GL_DEPTH_TEST|
GL_LIGHT0|GL_LIGHT1)`, `glDisable(same)`, `glFogi(GL_FOG_MODE)`,
`glFogf(GL_FOG_START|END|DENSITY)`, `glMultiTexCoord2f(GL_TEXTURE1, ...)`,
`glActiveTexture`, `glClientActiveTexture`, `glNewList`, `glEndList`,
`glCallList`, `glGenLists`, `glDeleteLists`, and the `glFog_4J` / `glLight_4J`
/ `glLightModel_4J` / `glCallLists_4J` templates at
`gl_compat.h:522-551` all route through `PlatformRenderer` methods even though
`PlatformRenderer` does not appear at the call site.

This means the audit must treat the matrix / state / fog / lighting /
command-buffer methods as being called from *every* file that does fixed-
function GL (which is nearly every file in `targets/minecraft/client/renderer`,
`targets/minecraft/client/gui`, `targets/minecraft/client/gui/inventory`,
`targets/minecraft/client/renderer/entity`, etc). The Phase 1 spec and the
Phase 2 legacy section of `IRenderPath` must cover the macro layer, not just
the direct `PlatformRenderer.` call sites.

## 2. Method-level inventory

Every virtual method on `IPlatformRenderer` with its caller shape and initial
disposition for the Phase 1 spec. "Direct" means a direct
`PlatformRenderer.method(...)` call site; "via macro" means the method is
reached exclusively or primarily through `gl_compat.h`.

| Method | Callers | Disposition |
|---|---|---|
| `Initialise` | main.cpp:438 (direct, 1) | Lifecycle. Factory construction. |
| `InitialiseContext` | GameRenderer.cpp:1120, LevelRenderer.cpp:4048 (direct, 2) | Lifecycle. Folds into factory. |
| `Tick` | main.cpp:535, GameNetworkManager.cpp:1518 (direct, 2) | Keep as host-callback. |
| `StartFrame` | main.cpp:522, Minecraft.cpp:403, UIController.cpp:965, GameNetworkManager.cpp:1512 (direct, 4) | Replaced by `render_frame` entry. |
| `Present` | main.cpp:584, Minecraft.cpp:444, GameNetworkManager.cpp:1521 (direct, 3) | Replaced by `render_frame` exit. |
| `Clear(int flags)` | UIController.cpp:1049 (direct, 1) | Reshape into `ViewDesc` clear-flags. |
| `SetClearColour` | no direct callers (implementation-internal; GameRenderer::setupClearColor uses glClearColor directly) | Reshape into `ViewDesc::clear_color`. |
| `Shutdown` | main.cpp:636 (direct, 1) | Destructor. |
| `Suspend` / `Suspended` / `Resume` | no direct callers in this repo | Lifecycle hook. Keep. |
| `SetWindowSize` | main.cpp:432 (direct, 1) | Host-layer, separate from `IRenderPath`. |
| `SetFullscreen` | main.cpp:433 (direct, 1) | Host-layer. |
| `IsWidescreen` | ~14 call sites across app/UI and Minecraft.cpp (direct, read-only) | Replaced by `FrameDesc::framebuffer.aspect` or equivalent. |
| `IsHiDef` | ~18 call sites across app/UI and Minecraft.cpp (direct, read-only) | Replaced by `FrameDesc::framebuffer.resolution_tier`. |
| `GetFramebufferSize` | main.cpp:525, Minecraft.cpp:1920, GameRenderer.cpp:1822, Screen.cpp:105, AchievementPopup.cpp:47 (direct, 5) | State reader. Replaced by `FrameDesc::framebuffer.{width,height}`. |
| `ShouldClose` / `Close` | main.cpp:521, Game.cpp:349, TitleScreen.cpp:178 (direct, 3) | Host-layer window lifecycle. |
| `UpdateGamma` | GameSettingsManager.cpp:261 (direct, 1) | Host-layer gamma ramp. |
| `MatrixMode` | via macro `glMatrixMode` (many) + gl_compat.h:346 | Reshape into per-`ViewDesc` camera matrices. |
| `MatrixSetIdentity` | via macro `glLoadIdentity` (many) + gl_compat.h:340 | Same. |
| `MatrixTranslate` | via macro `glTranslatef` (many) + gl_compat.h:303 | Same. |
| `MatrixRotate` | via macro `glRotatef` (many) + gl_compat.h:309 | Same. |
| `MatrixScale` | via macros `glScalef`/`glScaled` (many) + gl_compat.h:316,322 | Same. |
| `MatrixPerspective` | GameRenderer.cpp:645, GameRenderer.cpp:742, EnchantmentScreen.cpp:163 (direct, 3) | Reshape into `ViewDesc::projection`. |
| `MatrixOrthogonal` | via macro `glOrtho` (many) + gl_compat.h:458 | Reshape into `ViewDesc::projection`. |
| `MatrixPop` / `MatrixPush` | via macros `glPopMatrix`/`glPushMatrix` (many) + gl_compat.h:328,334 | Disappears - stack state becomes per-draw transforms. |
| `MatrixMult` | via macro `glMultMatrixf` + gl_compat.h:352 | Replaced by per-draw transform multiply. |
| `MatrixGet` | Frustum.cpp:61,63 (direct, 2 external); GLRenderer.cpp:1478 (internal GL wrapper) | State reader. See Section 5. |
| `Set_matrixDirty` | UIController.cpp:966 (direct, 1) | Internal state-sync marker. Delete. |
| `DrawVertices` | Tesselator.cpp:106,124,130,137 (direct, 4) | Reshape into `DrawCall` on a draw list. See Section 3. |
| `CBuffLockStaticCreations` | Minecraft.cpp:393 (direct, 1) | Keep as resource-lock hook, generalise. |
| `CBuffCreate` | no direct callers; reached via `glGenLists` macro (gl_compat.h:289) | Replaced by `create_mesh`. |
| `CBuffDelete` | no direct callers; reached via `glDeleteLists` macro (gl_compat.h:291) | Replaced by `destroy_mesh`. |
| `CBuffDeleteAll` | LevelRenderer.cpp:441 (direct, 1) | Replaced by resource-registry teardown. |
| `CBuffStart` | UIScene.cpp:564 (direct, 1) + via `glNewList` macro (gl_compat.h:277) | Replaced by `create_mesh` / `update_mesh` entry. |
| `CBuffClear` | Chunk.cpp:416,529,534,755 (direct, 4) | Replaced by `update_mesh(empty)` or `destroy_mesh`. |
| `CBuffSize` | LevelRenderer.cpp:1750 (direct, 1) | Budget-check query. Keep as query on the resource registry. |
| `CBuffEnd` | UIScene.cpp:585 (direct, 1) + via `glEndList` macro (gl_compat.h:279) | Matched pair with `CBuffStart`. |
| `CBuffCall` | LevelRenderer.cpp:867, UIScene.cpp:589 (direct, 2); via `glCallList` macro (gl_compat.h:285); via `glCallLists_4J` template (gl_compat.h:526); GLRenderer.cpp:1487 internal | Replaced by draw-list entry referencing a `MeshHandle`. |
| `CBuffTick` | no direct callers | Keep as implementation-internal cleanup tick. |
| `CBuffDeferredModeStart` | LevelRenderer.cpp:1965, LevelRenderer.cpp:2057 (direct, 2) | Replaced by "atomic update group" on the resource registry. See Section 7. |
| `CBuffDeferredModeEnd` | GameRenderer.cpp:1160 (direct, 1) | Matched pair with Start. |
| `TextureCreate` | GLRenderer.cpp:1432 internal only | Reshape into `create_texture`. |
| `TextureFree` | GLRenderer.cpp:1439 internal only | Reshape into `destroy_texture`. |
| `TextureBind` | Textures.cpp:536, HorseRenderer.cpp:62 (direct, 2) | Replaced by `DrawCall::texture_binding`. |
| `TextureBindVertex` | GameRenderer.cpp:807,843,847,1840 (direct, 4) | Lightmap binding. Replaced by per-view environment. |
| `TextureSetTextureLevels` | Texture.cpp:576, Textures.cpp:745 (direct, 2) | Part of `TextureDesc::mip_levels`. |
| `TextureGetTextureLevels` | Texture.cpp:599 (direct, 1) | State reader. Replace with caller-owned mip count. |
| `TextureData` | Texture.cpp:578,587, Textures.cpp:747,804, GLRenderer.cpp:1450 internal (direct, 5 external) | Reshape into `create_texture` + initial data. |
| `TextureDataUpdate` | Texture.cpp:595,604, Textures.cpp:881,904,924 (direct, 5) | Reshape into `update_texture(region)`. |
| `TextureSetParam` | no direct callers | Wraps GL `glTexParameter`. Fold into `TextureDesc`. |
| `TextureDynamicUpdateStart` / `End` | Textures.cpp:1149,1152 (both commented out) | Dead. Delete. |
| `LoadTextureData` (filename) | BufferedImage.cpp:92 (direct, 1) | File I/O helper. Move out of renderer interface into an `ImageLoader`. |
| `LoadTextureData` (memory) | BufferedImage.cpp:99,133,152 (direct, 3) | Same. |
| `SaveTextureData` / `SaveTextureDataToMemory` | no direct callers | Dead. Delete. |
| `ReadPixels` | GLRenderer.cpp:1493 internal only (reached from `glReadPixels_4J`) | State reader. Keep for screenshots / thumbnails behind explicit readback method. |
| `TextureGetStats` | no callers (stub at render_stubs.cpp) | Dead. Delete. |
| `TextureGetTexture` | no callers (stub, returns nullptr) | Dead. Delete. |
| `StateSetColour` | via macros `glColor4f` / `glColor3f` (many) + LevelRenderer.cpp:2231,2257,2267,2310 (direct, 4) + gl_compat.h:358,364 | Reshape into `DrawCall::tint_color`. |
| `StateSetDepthMask` | GameRenderer.cpp:1402,1459,1838 (direct, 3) | Reshape into `DrawCall::depth_write`. |
| `StateSetBlendEnable` | via macro `glEnable(GL_BLEND)` + gl_compat.h:385 | Reshape into `MaterialDesc::blend_mode`. |
| `StateSetBlendFunc` | GameRenderer.cpp:1400,1419,1423,1483,1646 (direct, 5) | Reshape into `MaterialDesc::blend_src/dst`. |
| `StateSetBlendFactor` | Gui.cpp:359,411 (direct, 2) | Constant blend factor. Reshape into `DrawCall::blend_factor`. |
| `StateSetAlphaFunc` | via macro `glAlphaFunc` + gl_compat.h:370 | Reshape into `MaterialDesc::alpha_func`. |
| `StateSetDepthFunc` | GameRenderer.cpp:1837 (direct, 1) | Reshape into `MaterialDesc::depth_func`. |
| `StateSetFaceCull` | GameRenderer.cpp:1830 (direct, 1) + via macro `glEnable/glDisable(GL_CULL_FACE)` + gl_compat.h:387,419 | Reshape into `MaterialDesc::cull_mode`. |
| `StateSetFaceCullCW` | no direct callers; implementation-internal | Reshape into `MaterialDesc::cull_winding`. |
| `StateSetLineWidth` | LevelRenderer.cpp:2232 (direct, 1) | Reshape into `DrawCall::line_width` (for wireframe entity bounds). |
| `StateSetWriteEnable` | GameRenderer.cpp:1264,1266,1517 (direct, 3) | Anaglyph channel masking. Reshape into `ViewDesc::color_mask`. |
| `StateSetDepthTestEnable` | via macro `glEnable/glDisable(GL_DEPTH_TEST)` + gl_compat.h:389,421 | Part of `MaterialDesc::depth_mode`. |
| `StateSetAlphaTestEnable` | via macro `glEnable/glDisable(GL_ALPHA_TEST)` + gl_compat.h:381,413 | Part of `MaterialDesc::alpha_mode`. |
| `StateSetDepthSlopeAndBias` | no direct callers | Polygon offset. Keep as `DrawCall::depth_bias`. |
| `StateSetFogEnable` | via macro `glEnable/glDisable(GL_FOG)` + gl_compat.h:377,409 | Reshape into `ViewDesc::fog.enabled`. |
| `StateSetFogMode` | via macro `glFogi(GL_FOG_MODE)` + gl_compat.h:441 | Reshape into `ViewDesc::fog.mode`. |
| `StateSetFogNearDistance` / `FarDistance` / `Density` | via macros `glFogf(GL_FOG_START|END|DENSITY)` + gl_compat.h:448,450,452 | Reshape into `ViewDesc::fog.{start,end,density}`. |
| `StateSetFogColour` | via `glFog_4J` template at gl_compat.h:533 + GLRenderer.cpp:1474 internal | Reshape into `ViewDesc::fog.color`. |
| `StateSetLightingEnable` | LevelRenderer.cpp:2227,2259,2265,2308 (direct, 4) + via macro `glEnable/glDisable(GL_LIGHTING)` + gl_compat.h:379,411 | Reshape into `MaterialDesc::lit` flag on each draw. |
| `StateSetVertexTextureUV` | via macro `glMultiTexCoord2f(GL_TEXTURE1)` + gl_compat.h:466 | Lightmap UV. Reshape into `DrawCall::lightmap_uv` or per-vertex data. |
| `StateSetLightColour` | via `glLight_4J` template at gl_compat.h:544 + GLRenderer.cpp:1460 internal | Reshape into `ViewDesc::lighting.directional_color[i]`. |
| `StateSetLightAmbientColour` | via `glLight_4J` + `glLightModel_4J` templates at gl_compat.h:542,551 + GLRenderer.cpp:1462,1468 internal | Reshape into `ViewDesc::lighting.ambient_color`. |
| `StateSetLightDirection` | via `glLight_4J` template at gl_compat.h:539 + GLRenderer.cpp:1458 internal | Reshape into `ViewDesc::lighting.directional_dir[i]`. |
| `StateSetLightEnable` | via macro `glEnable/glDisable(GL_LIGHT0|GL_LIGHT1)` + gl_compat.h:391,393,423,425 | Reshape into `ViewDesc::lighting.directional_enabled[i]`. |
| `StateSetViewport` | Minecraft.cpp:1634,1663,1673,1686 (direct, 4) | Reshape into `FrameDesc::views[i].viewport_layout`. See Section 6. |
| `StateSetEnableViewportClipPlanes` | LevelRenderer.cpp:1436,1699, GameRenderer.cpp:1616,1816 (direct, 4) | Scissor rect under a different name. Reshape into `ViewDesc::scissor_enable`. |
| `StateSetTexGenCol` | no direct callers; implementation-internal (used by projected-texture shader path) | Reshape into `MaterialDesc` projected-texture params. |
| `StateSetStencil` | UIScene_SkinSelectMenu.cpp:529 (direct, 1) | Reshape into `DrawCall::stencil_op`. |
| `StateSetForceLOD` | ItemInHandRenderer.cpp:303,365, ItemRenderer.cpp:225,314 (direct, 4) | Reshape into `DrawCall::forced_lod`. |
| `StateSetTextureEnable` | via macro `glEnable/glDisable(GL_TEXTURE_2D)` + gl_compat.h:383,415 | Reshape into `MaterialDesc::textured` flag. |
| `StateSetActiveTexture` | via macros `glActiveTexture`/`glClientActiveTexture` + gl_compat.h:472,478 | Disappears - multi-texture binding becomes `DrawCall::texture_bindings[slot]`. |
| `SetChunkOffset` | LevelRenderer.cpp:863,872 (direct, 2) | Per-draw chunk offset. Reshape into `ChunkDrawEntry::chunk_offset`. See Section 10. |
| `BeginConditionalSurvey` / `EndConditionalSurvey` / `BeginConditionalRendering` / `EndConditionalRendering` | no direct callers in this repo; occlusion-query path is either disabled or implementation-internal | Keep as optional occlusion-query contract if still wanted; otherwise dead. |
| `DoScreenGrabOnNextPresent` | no direct callers | Dead. Delete. |
| `CaptureThumbnail` | no direct callers (gameServices().captureSaveThumbnail() exists but routes elsewhere) | Investigate during Phase 1, likely move out of renderer. |
| `CaptureScreen` | no direct callers | Investigate during Phase 1. |
| `BeginEvent` / `EndEvent` | no direct callers | PIX/RenderDoc markers. Keep. |

Total: 106 virtual methods (counting `LoadTextureData` as two overloads).
After the audit, 8 methods are confirmed dead and eligible for immediate
deletion (see Section 12).

## 3. `DrawVertices` vertex/material matrix

All four direct `DrawVertices` call sites are in
`targets/minecraft/client/renderer/Tesselator.cpp` in the `Tesselator::end()`
implementation. There is no other draw submission path in the game - every
visible triangle in 4jcraft flows through this function.

### 3.1 Call sites

```
Tesselator.cpp:106  (GL_QUADS with TRIANGLE_MODE):
    DrawVertices(
        PRIMITIVE_TYPE_TRIANGLE_LIST, vertices, _array->data(),
        useCompactFormat360 ? VERTEX_TYPE_COMPRESSED
                             : VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1,
        useProjectedTexturePixelShader ? PIXEL_SHADER_TYPE_PROJECTION
                                       : PIXEL_SHADER_TYPE_STANDARD);

Tesselator.cpp:124  (compact, non-QUADS):
    DrawVertices(mode, vertexCount, _array->data(),
        VERTEX_TYPE_COMPRESSED, PIXEL_SHADER_TYPE_STANDARD);

Tesselator.cpp:130  (projected, non-QUADS):
    DrawVertices(mode, vertexCount, _array->data(),
        VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN,
        PIXEL_SHADER_TYPE_PROJECTION);

Tesselator.cpp:137  (standard, non-QUADS):
    DrawVertices(mode, vertexCount, _array->data(),
        VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1,
        PIXEL_SHADER_TYPE_STANDARD);
```

### 3.2 Unique (vertex layout, pixel shader) pairs

Five pairs are reachable at runtime (not three, as earlier recon suggested -
the site at line 106 produces two pairs on each of its two conditional
branches).

| Vertex layout | Shader | Reachable at | `MaterialDesc` intent |
|---|---|---|---|
| `VERTEX_TYPE_COMPRESSED` | `STANDARD` | Tesselator.cpp:106 (compact quad path, no projection), 124 | Chunk geometry compact path, standard lighting/fog |
| `VERTEX_TYPE_COMPRESSED` | `PROJECTION` | Tesselator.cpp:106 (compact quad path, projection) | Chunk geometry compact path + projected texture (selection, shadow, or decal) |
| `VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1` | `STANDARD` | Tesselator.cpp:106 (full quad path, no projection), 137 | Uncompressed geometry, standard lighting/fog. The default path for non-chunk draws. |
| `VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1` | `PROJECTION` | Tesselator.cpp:106 (full quad path, projection) | Uncompressed geometry + projected texture |
| `VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_TEXGEN` | `PROJECTION` | Tesselator.cpp:130 | Uncompressed geometry + per-vertex texture-gen data + projected texture |

### 3.3 Runtime state each pair reads

All `DrawVertices` calls happen after `Tesselator::end()` has populated the
vertex buffer. They read the following ambient state that the caller set
through `PlatformRenderer` / `gl_compat.h` before calling
`Tesselator::end()`:

- Matrix stack (modelview + projection) - from any `glPushMatrix` / `glMultMatrixf` / `glTranslatef` sequence.
- Fog enable / mode / start / end / density / colour - set per-frame by `GameRenderer::setupFog` (see Section 9).
- Lighting enable / per-light direction / per-light colour / ambient - set by `GameRenderer::turnOnLightLayer` / `turnOffLightLayer`.
- Alpha test enable + alpha func - set by entity and block renderers, most commonly `glAlphaFunc(GL_GREATER, 0.1f)`.
- Blend enable + blend func - set by `GameRenderer::render` around the translucent pass (`GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA` for normal, `GL_SRC_ALPHA`, `GL_ONE` for particles, `GL_ZERO`, `GL_ONE` for anaglyph mask).
- Depth test enable + depth func + depth write - set by `GameRenderer::render` and per-call by `ItemRenderer`, `EntityRenderer`.
- Cull enable + winding - usually on, disabled for transparent/two-sided tiles.
- Active texture + currently bound texture ID - set by `Textures::bindAndUpload` and direct `glBindTexture` from renderer code.
- Chunk offset (via `SetChunkOffset`) - used exclusively on the chunk replay path.

Every one of these has to become a field on `MaterialDesc`, `DrawCall`, or
`ViewDesc` in Phase 1. None of them can remain as ambient renderer state
after Phase 7. The current runtime-branch structure of
Tesselator.cpp:106-144 is itself a useful hint: the three axes that actually
vary at draw time are vertex-layout compression, projection-shader on/off,
and primitive type. A material system that captures those three axes covers
the whole draw surface.

### 3.4 Enum values confirmed unused

- `VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT` is never passed to `DrawVertices`.
  The "_LIT" suffix suggests a dedicated pre-lit vertex path that the game
  does not actually ship. Dead; delete in Phase 2.
- `PIXEL_SHADER_TYPE_FORCELOD` is never passed to `DrawVertices`. The
  `StateSetForceLOD` method is used (see Section 8), but it does not change
  which shader is selected - it only biases LOD selection in the shader.
  Dead as a shader selector; delete the enum value and keep
  `StateSetForceLOD` under a different name for Phase 1.
- `PRIMITIVE_TYPE_QUAD_LIST` is passed via the macro path
  `(ePrimitiveType)mode` where `mode = GL_QUADS`, but the code at line 104
  specifically unrolls `GL_QUADS` to `PRIMITIVE_TYPE_TRIANGLE_LIST` when
  `TRIANGLE_MODE` is defined. Whether `QUAD_LIST` is reachable at all
  depends on the `TRIANGLE_MODE` define. If it is always defined
  (expected for modern GL core), `QUAD_LIST` is dead; confirm in Phase 1.

## 4. Texture usage patterns

### 4.1 Creation and lifetime

`TextureCreate` / `TextureFree` are only called by the
`glGenTextures_4J` / `glDeleteTextures_4J` helpers in
`GLRenderer.cpp:1432,1439`, which are themselves invoked by game code via the
`glGenTextures` / `glDeleteTextures` macros. External callers therefore see
this as "allocate a texture handle" / "free it". `create_texture` replaces
both.

### 4.2 Data upload

- `TextureData(width, height, data, level, format)` creates full mip levels.
  Called from:
  - `Texture.cpp:578, 587` - loading baked textures from resource pack.
  - `Textures.cpp:747, 804` - loading texture atlas and dynamic textures.
- `TextureDataUpdate(x, y, width, height, data, level)` patches a sub-region.
  Called from:
  - `Texture.cpp:595, 604` - per-frame animated-tile updates (water, lava, portal, fire, compass, clock).
  - `Textures.cpp:881` - dynamic sky-tint texture region.
  - `Textures.cpp:904, 924` - lightmap update (see 4.5).
- `TextureSetTextureLevels(n)` is called by `Texture.cpp:576` and
  `Textures.cpp:745` prior to `TextureData` to tell the backend how many mip
  levels are about to be uploaded. This is a per-texture property and
  belongs inside `TextureDesc::mip_levels`.

### 4.3 Binding

- `TextureBind(idx)` - single texture-unit bind. Called from
  `Textures.cpp:536` (general atlas bind) and `HorseRenderer.cpp:62` (horse
  armour texture override).
- `TextureBindVertex(idx, scaleLight)` - second texture unit (lightmap slot).
  Called from `GameRenderer.cpp:807, 843, 847, 1840`. These four sites are
  the entire lightmap binding flow: unbind, bind current lightmap scaled,
  bind current lightmap unscaled, unbind before swap. `scaleLight` encodes
  a secondary behaviour that only matters for the lightmap texture.
- `StateSetActiveTexture(tex)` via the `glActiveTexture` macro selects which
  texture unit a subsequent single-unit `glBindTexture` applies to. This is
  fixed-function GL multitexture state; Phase 1 replaces it with explicit
  per-slot binding on `DrawCall::texture_bindings[slot]`.

### 4.4 Dynamic updates

`TextureDynamicUpdateStart` / `TextureDynamicUpdateEnd` exist in the interface
but the only call sites (`Textures.cpp:1149, 1152`) are commented out. Dead.

### 4.5 Lightmap

The lightmap is a 16x16 RGBA8 texture updated every frame by
`Textures::updateLightmapTexture` (`Textures.cpp:904, 924` - two calls, one
for the main pass, one for the night-vision pass). It is bound through
`TextureBindVertex`, not through `TextureBind`, because it lives in texture
unit 1. This is the one texture where "updated every frame, read on the hot
path" is the actual usage pattern. `TextureDesc::usage = dynamic_stream`
plus `update_texture(region)` called from the main thread covers this.

### 4.6 File I/O

`LoadTextureData` takes either a filename or a memory blob, plus a
`D3DXIMAGE_INFO*` output, and returns a decoded pixel buffer. Called from
`BufferedImage.cpp:92, 99, 133, 152`. This is a PNG decoder behind the
renderer interface. It should not be in `IRenderPath`. Phase 1 moves it into
a separate `ImageLoader` / `stb_image` helper outside the renderer.

`D3DXIMAGE_INFO` is defined in `targets/platform/PlatformTypes.h` and is a
legacy type name borrowed from the Direct3D 9 D3DX helper library; the
struct just carries width / height / format / mip count. It should not
appear in a modern renderer interface either.

`SaveTextureData` / `SaveTextureDataToMemory` have no callers. Dead.

### 4.7 Readback

`ReadPixels(x, y, w, h, buf)` has no direct external callers. Its single
caller is the `glReadPixels_4J` wrapper at `GLRenderer.cpp:1493`, which
exists to support screenshot / thumbnail capture. Phase 1 exposes readback
as an explicit method on the resource registry (`read_framebuffer(region,
out_buffer)`) rather than as a raw `ReadPixels`. The thread and timing
contract must be spelled out (blocking, main thread only).

## 5. State readers

These are the hardest-to-migrate call sites because they imply that the
caller does not own the data it needs.

| Caller | Method | Data read | Migration note |
|---|---|---|---|
| main.cpp:525 | `GetFramebufferSize(fbw, fbh)` | framebuffer width/height | Framebuffer size is already host-knowable. Pass as explicit parameter into whatever main.cpp does next. |
| Minecraft.cpp:1920 | `GetFramebufferSize(fbw, fbh)` | framebuffer width/height | Same. Minecraft::setDisplaySize is the caller; it can take width/height arguments. |
| GameRenderer.cpp:1822 | `GetFramebufferSize(fbw, fbh)` | framebuffer width/height | Called inside `GameRenderer::renderBetweenEffects`. Pass from the `FrameDesc` that the eventual render path receives. |
| Screen.cpp:105 | `GetFramebufferSize(fbw, fbh)` | framebuffer width/height | Screen layout. Pass from `FrameDesc::framebuffer`. |
| AchievementPopup.cpp:47 | `GetFramebufferSize(fbw, fbh)` | framebuffer width/height | Same. |
| Frustum.cpp:61 | `MatrixGet(GL_PROJECTION_MATRIX)` | current projection matrix | The comment at Frustum.cpp:59 explicitly says `Camera::prepare()` already captures both matrices every frame. Rewire `Frustum::calculateFrustum` to take matrices from `Camera` directly instead of re-querying the renderer. This is the migration path and it is already partially done - the code just needs to flip from the renderer-query path to the `Camera`-held path. |
| Frustum.cpp:63 | `MatrixGet(GL_MODELVIEW_MATRIX)` | current modelview matrix | Same. |
| GLRenderer.cpp:1478 | `MatrixGet(...)` | any matrix, used by `glGetDoublev` wrapper | Internal only. Not a migration target. |
| Texture.cpp:599 | `TextureGetTextureLevels()` | mip levels on currently bound texture | Refactor to pass mip level count alongside the texture handle in the caller, which already knows because it set the value via `TextureSetTextureLevels` on the same line. |
| GLRenderer.cpp:1493 | `ReadPixels(...)` | framebuffer pixels | Internal GL wrapper. See Section 4.7. |

The `IsHiDef` / `IsWidescreen` queries count as state readers too but they
read window config rather than GPU state and are trivially replaced by
cached values on a window / framebuffer object.

### 5.1 Frustum construction, expanded

`targets/minecraft/client/renderer/culling/Frustum.cpp` is the single
hardest external state-reader site. `Frustum::calculateFrustum` (line 56)
reads the current projection and modelview matrices and multiplies them to
build the six clip planes. The code then extracts plane normals from the
combined matrix (lines 66-148) using the standard Hartmann-Gribb method.

The in-code comment at line 57-60 explicitly says:
> GL 3.3 core removed GL_MODELVIEW_MATRIX / GL_PROJECTION_MATRIX queries.
> Camera::prepare() already captures both matrices every frame :)
> i spent an ungodly amount of time on this simple fix.

This means the renderer already stores a shadow copy of the two matrices in
its GL implementation specifically to support this query. `MatrixGet` in
the current backend is a shadow-state read, not a GL read. That makes the
migration cheap: move the shadow copy out of the renderer and into
`Camera`, and call `Frustum::calculateFrustum(const mat4& proj, const
mat4& view)` with them as parameters. No interface method needed.

## 6. Viewport and view state

### 6.1 Live viewport types

Contrary to the earlier recon report, all nine viewport types are live at
runtime. The confusion was that the direct literal enum names only appear
as `FULLSCREEN` and the `QUADRANT_*` values at `StateSetViewport` call
sites, but the split values reach `StateSetViewport` through an
`(IPlatformRenderer::eViewportType)player->m_iScreenSection` cast at
`Minecraft.cpp:1634`.

`m_iScreenSection` is an `int` member of `LocalPlayer` and is assigned
from the enum at:

- `Minecraft.cpp:700-701` - single viewport: `VIEWPORT_TYPE_FULLSCREEN`.
- `Minecraft.cpp:712-713` - 2-player vertical split: `VIEWPORT_TYPE_SPLIT_LEFT + found`.
- `Minecraft.cpp:715-716` - 2-player horizontal split: `VIEWPORT_TYPE_SPLIT_TOP + found`.
- `Minecraft.cpp:747-748`, 763-766, 892, 4007 - 3-4 player quadrants: `VIEWPORT_TYPE_QUADRANT_TOP_LEFT + found`.
- `LocalPlayer.cpp:125` - initial value.

So the real live set is:
- `FULLSCREEN` (1 player or frontend).
- `SPLIT_TOP` / `SPLIT_BOTTOM` (2 player horizontal split, chosen when `eGameSetting_SplitScreenVertical` is false).
- `SPLIT_LEFT` / `SPLIT_RIGHT` (2 player vertical split, chosen when `eGameSetting_SplitScreenVertical` is true).
- `QUADRANT_TOP_LEFT` / `QUADRANT_TOP_RIGHT` / `QUADRANT_BOTTOM_LEFT` / `QUADRANT_BOTTOM_RIGHT` (3-4 player split).

All nine values survive into Phase 1. They can be collapsed into a
`ViewportLayout` enum with explicit `{ which_player, total_players,
orientation_preference }`, but the specific screen rectangles those map to
must still be expressible per-view in `FrameDesc`.

### 6.2 Viewport call sites

```
Minecraft.cpp:1634  StateSetViewport((eViewportType)player->m_iScreenSection)
    -> per-player, inside the player render loop.
Minecraft.cpp:1663-1664  StateSetViewport(FULLSCREEN)
    -> fallback when no player rendered (frontend path).
Minecraft.cpp:1673-1677  StateSetViewport((eViewportType)(QUADRANT_TOP_LEFT + unoccupiedQuadrant))
    -> clears a black quadrant for the "empty player slot" case in 3-player splitscreen.
Minecraft.cpp:1686-1687  StateSetViewport(FULLSCREEN)
    -> resets viewport after all players rendered.
```

### 6.3 Clip planes

`StateSetEnableViewportClipPlanes(true/false)` at LevelRenderer.cpp:1436,
1699, GameRenderer.cpp:1616, 1816. These are toggled around passes that
must clip to the current viewport quadrant - specifically the world
rendering and the item-in-hand rendering. This is a scissor rect, not GL
user clip planes. Reshape as `ViewDesc::scissor_from_viewport = true`.

## 7. Lifecycle and threading

### 7.1 Chunk rebuild dispatch

Entry point: `LevelRenderer::updateDirtyChunks()` at
`targets/minecraft/client/renderer/LevelRenderer.cpp:1702`.

Dispatch logic at lines 1992-2032:

```
if (veryNearCount > 0)
    bAtomic = true;  // rebuild on main thread atomically
if (bAtomic || (index == 0))
    permaChunk[index].rebuild();   // main thread, synchronous
else
    s_activationEventA[index - 1]->set();  // signal worker thread
```

The main-thread atomic path exists specifically to avoid visible holes when
a cluster of adjacent chunks all need rebuilding at once. The comment at
lines 1955-1963 explains: `CBuffDeferredModeStart` moves the backend into
deferred commit mode so the new command buffers for the near group don't
go live until `CBuffDeferredModeEnd` is called, at which point all the
adjacent-chunk command buffers flip in together. This is the one piece of
atomic-update semantics in the whole codebase and the future resource
registry must preserve it.

Main-thread and worker-thread paths both end up calling
`Chunk::rebuild()`, which in turn calls the command-buffer start/end
sequence:

```
glNewList(lists + currentLayer, GL_COMPILE);   // -> CBuffStart(lists + currentLayer)
... Tesselator fills vertex data, eventually calls DrawVertices ...
glEndList();                                    // -> CBuffEnd()
```

### 7.2 Worker-thread GL context

`MAX_CHUNK_REBUILD_THREADS` worker threads are each paired with an
`s_activationEventA[i]` event. Workers wait on their event; main thread
sets the event to kick off a rebuild. Each worker has:

- A thread-local `Tesselator` instance (`Tesselator.cpp:29`:
  `thread_local Tesselator* Tesselator::m_tlsInstance`).
- A thread-local tile-id scratch buffer (`Chunk.cpp:31-34`:
  `thread_local uint8_t* Chunk::m_tlsTileIds`, allocated by
  `Chunk::CreateNewThreadStorage` on worker startup).
- Access to the global `PlatformRenderer` macro, which on the GL backend
  is assumed to have a thread-local or mutex-protected GL context
  compatible with chunk rebuild.

The `bAtomic` escape is the only path that routes a worker-owned rebuild
back onto the main thread, and it only fires when `veryNearCount > 0`
(i.e., when several chunks within a few blocks of the player all need
rebuilding). All other rebuilds run on worker threads.

### 7.3 Methods called from worker threads today

From the chunk rebuild hot path, the following `PlatformRenderer` calls
happen on worker threads:

- `CBuffClear(lists + layer)` - `Chunk.cpp:416, 529, 534, 755`.
- `CBuffStart(lists + layer)` - via `glNewList` macro at `Chunk.cpp:479`.
- `CBuffEnd()` - via `glEndList` macro at `Chunk.cpp:512`.
- `DrawVertices(...)` - via `Tesselator::end()` which is called inside
  `Chunk.cpp:510` (`t->end()`).
- `StateSetColour`, `StateSetBlendEnable`, etc. - any state the tile
  renderers set while building geometry, which flows through the
  `gl_compat.h` macros inside the tile / mob / entity renderers called
  during `tesselateInWorld`.

The new `IRenderPath` thread contract must allow every one of these to be
called from a worker thread. The resource registry is the most important
case: `create_mesh` / `update_mesh` / `destroy_mesh` must be callable from
any thread.

### 7.4 Main-thread lifecycle

```
main.cpp:438  PlatformRenderer.Initialise()        // create window + GL context
main.cpp:521  while (!PlatformRenderer.ShouldClose())
main.cpp:522      PlatformRenderer.StartFrame()
main.cpp:535      PlatformRenderer.Tick()
Minecraft.cpp:393 PlatformRenderer.CBuffLockStaticCreations()
                  // chunk updates, rendering, GUI
main.cpp:584      PlatformRenderer.Present()
main.cpp:636  PlatformRenderer.Shutdown()
```

`CBuffLockStaticCreations` at `Minecraft.cpp:393` is called after initial
resource creation to mark the point where static buffers become immutable
and future allocations go into a different pool. This is a budget/tiering
hint for `CBuffSize` (`LevelRenderer.cpp:1750`). The new resource registry
can keep this concept as `resource_registry.seal_static_tier()` or fold it
into a resource-lifetime hint on `create_mesh`.

## 8. Matrix stack usage

There are three direct `PlatformRenderer.Matrix*` call sites outside
`gl_compat.h` and the GL implementation:

- `GameRenderer.cpp:645` - `MatrixPerspective(fov, aspect, 0.05f, renderDistance * 2)` in `GameRenderer::setupCamera`.
- `GameRenderer.cpp:742` - same, in `GameRenderer::renderItemInHand`.
- `EnchantmentScreen.cpp:163` - `MatrixPerspective(90.0f, 1.3333334f, 9.0f, 80.0f)` for the enchantment table 3D preview.
- `Frustum.cpp:61, 63` - `MatrixGet`, see Section 5.
- `UIController.cpp:966` - `Set_matrixDirty`, marker only.

All other matrix-stack usage is via `gl_compat.h` macros. Grouped by
subsystem:

### 8.1 World setup (`GameRenderer.cpp`)

`setupCamera(a, eye)` at line 627 is the primary world matrix builder:

```
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
if (anaglyph3d) glTranslatef(-(eye*2-1)*stereoScale, 0, 0);
if (zoom != 1) { glTranslatef(zoom_x, -zoom_y, 0); glScaled(zoom, zoom, 1); }
PlatformRenderer.MatrixPerspective(fov, aspect, 0.05f, renderDistance*2);
if (isCutScene) glScalef(1, 1/1.5f, 1);

glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
if (anaglyph3d) glTranslatef((eye*2-1)*0.10f, 0, 0);
bobHurt(a);
if (ViewBob enabled) bobView(a);
[portal effect]
moveCameraToPlayer(a);
[camera flip rotations]
```

Every line above needs to become a field in `ViewDesc`:

- `fov` / `aspect` / `renderDistance` -> `ViewDesc::projection`
- `anaglyph3d` + `eye` -> `ViewDesc::stereo_eye_offset`
- `zoom` + `zoom_x` / `zoom_y` -> `ViewDesc::zoom` (2D screen-space offset)
- Cutscene y-squash -> `ViewDesc::projection_post_scale`
- `bobHurt` / `bobView` / portal effect / camera flip -> `ViewDesc::view_matrix` (computed CPU-side from camera state)
- `moveCameraToPlayer` -> part of `ViewDesc::view_matrix`

`renderItemInHand(a, eye)` at line 698 does the same but with its own
projection (different near plane) and a separate modelview that isolates
the hand from the world. This fits as a second `ViewDesc` entry in the
same frame: a small overlay view drawn after the world view with its own
camera. The view list on `FrameDesc` already has to support this.

### 8.2 Entity rendering

Entity renderers (`EntityRenderer`, `MobRenderer`, `ItemRenderer`,
`HorseRenderer`, etc) each wrap their rendering in
`glPushMatrix` / `glTranslatef` / `glRotatef` / `glPopMatrix` around a
Tesselator draw. This pattern is everywhere. The migration is to capture
those transforms as a `mat4` on each `DrawCall` entry instead of pushing
them onto an implicit stack.

### 8.3 GUI / HUD / text

`Gui.cpp`, `Screen.cpp`, and the item-in-inventory renderers build a 2D
projection via `glOrtho` + `glLoadIdentity` and then emit screen-space
quads via Tesselator. Same pattern - becomes
`FrameDesc::ui_draw_list` with per-draw `mat4` or screen-space rect.

### 8.4 Inventory 3D preview

`EnchantmentScreen.cpp:163` is one of a handful of GUI screens that
switches to a perspective projection to render a 3D object in-place
(enchantment table book animation, crafting table 3D item icon, etc).
These need either a second `ViewDesc` entry per frame (if scissored to
the widget rect) or a hybrid "3D draw into UI" path. Phase 1 decides
which; both are feasible.

## 9. Fixed-function fog and lighting

### 9.1 Fog

Fog is set per-frame in `GameRenderer::setupFog(i, alpha)`
(`GameRenderer.cpp:2018`). Data flow:

- `glFog(GL_FOG_COLOR, getBuffer(fr, fg, fb, 1))` -> `glFog_4J` -> `StateSetFogColour`.
- `glFogi(GL_FOG_MODE, GL_LINEAR|GL_EXP)` -> `StateSetFogMode`.
- `glFogf(GL_FOG_START, ...)` -> `StateSetFogNearDistance`.
- `glFogf(GL_FOG_END, ...)` -> `StateSetFogFarDistance`.
- `glFogf(GL_FOG_DENSITY, ...)` -> `StateSetFogDensity`.

Data sources (all read by `setupFog`):

- Blindness effect on the camera target (mode=LINEAR, start=0, end=distance*0.8 during fade-in, distance*0.25/distance after).
- Cloud immersion (`isInClouds`) (mode=EXP, density=0.1).
- Underwater (tile at camera = water) (mode=EXP, density=0.05-0.1 depending on water breathing and oxygen bonus).
- In-lava (tile at camera = lava) (mode=EXP, density=2.0).
- Open air (mode=LINEAR, start=renderDistance*0.25, end=renderDistance).
- Dimension has bedrock fog + creative + low Y (linear start adjusted).
- Dimension `isFoggyAt(x,z)` (denser linear fog).

This is the entire fog model. It fits in a `ViewDesc::fog` struct with the
fields `{ enabled, mode, color, start, end, density }`. The fog branch on
`i == -1` / `i == 0` / `i == 1` (sky fog / world fog / item-in-hand fog)
means there is a *per-draw-pass* fog choice inside the same view, so the
struct needs either multiple fog profiles per view or fog-override fields
on individual draws. Phase 1 decides which.

The final `glEnable(GL_COLOR_MATERIAL)` + `glColorMaterial(GL_FRONT, GL_AMBIENT)`
at the end of `setupFog` (lines 2133-2134) binds vertex color to material
ambient, which is part of the lighting model, not fog. Move it to the
lighting setup section of `ViewDesc`.

### 9.2 Lighting

Directional light setup happens in `GameRenderer::turnOnLightLayer` (line
827) and gets torn down in `turnOffLightLayer` (line 802). Internally
these use `glLight` / `glLightModel` templates that route to
`StateSetLightDirection` / `StateSetLightColour` /
`StateSetLightAmbientColour`, plus direct `StateSetLightingEnable(true)`.

Light 0 and Light 1 are two directional lights with fixed directions
(sun / moon / anti-sun depending on time of day). Ambient is computed
from the current sky light level.

Data sources:

- Sky brightness table (function of time of day).
- Block light level at the eye position (for lightmap tint, separate from
  directional lights).
- Night-vision potion effect (boosts ambient).
- Underwater tint (boosts blue channel on directional + ambient).

The lightmap texture (Section 4.5) is the actual per-pixel lighting; the
two directional lights are a fixed supplementary contribution. In the
new contract these are:

- Two directional-light slots on `ViewDesc::lighting`.
- A lightmap texture handle also on `ViewDesc::lighting`.
- A `MaterialDesc::lit` flag so individual draws can opt out (e.g. sky, GUI).

`StateSetLightingEnable(false/true)` direct calls at LevelRenderer.cpp:
2227, 2259, 2265, 2308 correspond to the bounding-box-debug and
break-progress wireframe passes, which opt out of lighting. Those become
per-draw `lit: false`.

## 10. Command buffer usage

### 10.1 Static chunk geometry

This is the load-bearing case. `Chunk::rebuild()` in `Chunk.cpp:455` builds
per-chunk geometry into a pair of command buffer slots (one per render
layer, opaque vs transparent):

```
for (currentLayer = 0..1):
    glNewList(lists + currentLayer, GL_COMPILE);     // CBuffStart
    t->begin(); t->offset(-x, -y, -z);
    [iterate tiles, call tileRenderer->tesselateInWorld, which calls t->addVertex etc]
    t->end();                                         // -> DrawVertices
    glEndList();                                      // CBuffEnd
```

Empty layers are cleared with `PlatformRenderer.CBuffClear(lists + layer)`
(`Chunk.cpp:416, 529, 534, 755`).

Chunks are replayed at render time in `LevelRenderer::render` via
`LevelRenderer::renderSorted` (`LevelRenderer.cpp:855-873`):

```
for each visible chunk:
    PlatformRenderer.SetChunkOffset(chunk->x, chunk->y, chunk->z);
    PlatformRenderer.CBuffCall(list, first);
    first = false;
PlatformRenderer.SetChunkOffset(0, 0, 0);
```

The comment at line 861 explains: this replaced per-chunk `glPushMatrix` /
`glTranslatef` / `glPopMatrix` for chunk positioning with an explicit
per-draw chunk offset. `SetChunkOffset` is therefore a per-draw transform
piggybacked on a single-float-vec3 uniform, not a matrix stack operation.
In Phase 1 it becomes `ChunkDrawEntry::chunk_offset`.

Key properties to preserve:
- `CBuffStart`/`CBuffEnd` may be called from worker threads (see Section 7).
- `CBuffCall` is main-thread-only.
- `CBuffCall` returns `bool` to indicate whether the buffer was actually
  committed; callers use this to decide whether it's the first draw of
  the frame. Keep the return.
- `CBuffClear` frees the slot's contents but keeps the slot allocated.
- `CBuffDeferredModeStart`/`End` brackets an atomic update group
  (Section 7.1).
- `CBuffSize(-1)` returns the total allocation footprint across all
  command buffers, used by `LevelRenderer::updateDirtyChunks` as a
  back-pressure signal.

### 10.2 UI command buffers

`UIScene.cpp:564-589`:

```
if (useCommandBuffers)
    PlatformRenderer.CBuffStart(list, true);
... emit UI geometry ...
if (useCommandBuffers) PlatformRenderer.CBuffEnd();
... later ...
if (useCommandBuffers) (void)PlatformRenderer.CBuffCall(list);
```

This is the GUI subsystem caching its own geometry in command buffers as
an optimisation. Phase 3 (GUI migration) subsumes this by moving GUI into
the `FrameDesc::ui_draw_list` path. The caching becomes a property of the
UI renderer, not of the platform renderer.

### 10.3 Display-list allocation

`CBuffCreate(range)` via `glGenLists(range)` and
`CBuffDelete(first, count)` via `glDeleteLists(first, range)` are the
allocator. Callers use `glGenLists(2)` to allocate two slots per chunk
(one per layer). In Phase 1, `create_mesh` returns a handle; the two-slot
per-chunk scheme becomes two separate handles.

`CBuffDeleteAll` at `LevelRenderer.cpp:441` is called during level
teardown. `destroy_all_meshes` or a registry teardown is the equivalent.

## 11. App / UI caller surface

The `targets/app/` tree contains 28 .cpp files and several headers that
reference `PlatformRenderer`. Most are read-only queries for layout
decisions (`IsHiDef`, `IsWidescreen`); a smaller set do real rendering
work.

### 11.1 Read-only resolution/aspect queries (trivial migration)

These just branch layout on SD vs HD, 4:3 vs 16:9. They become reads from
`FrameDesc::framebuffer.resolution_tier` / `aspect_hint`:

- `targets/app/common/LocalizationManager.cpp:111`
- `targets/app/common/DLC/DLCSkinFile.cpp:59-60`
- `targets/app/common/DLC/DLCAudioFile.cpp:103-104`
- `targets/app/common/Game.cpp:541`
- `targets/app/common/UI/UIScene.cpp:147,161`
- `targets/app/common/UI/UIBitmapFont.cpp:277` (commented)
- `targets/app/common/UI/Components/UIComponent_Tooltips.cpp:58`
- `targets/app/common/UI/Scenes/Frontend Menu screens/UIScene_LaunchMoreOptionsMenu.cpp:214, 503`
- `targets/app/common/UI/Scenes/Frontend Menu screens/UIScene_LoadMenu.cpp:253`
- `targets/app/common/UI/Scenes/Frontend Menu screens/UIScene_DLCOffersMenu.cpp:37`
- `targets/app/common/UI/Scenes/Help & Options/UIScene_SettingsOptionsMenu.cpp:98, 203, 302, 383-384`
- `targets/app/common/UI/All Platforms/IUIScene_AbstractContainerMenu.cpp:331`
- `targets/app/common/UI/All Platforms/IUIScene_CraftingMenu.cpp:483, 1038, 1317-1318`
- `targets/app/common/UI/All Platforms/IUIScene_CreativeMenu.cpp:881` (commented)

Headers that surface these queries to their .cpp through inline definitions:
`UIScene.h`, `UIController.h`, `UIGroup.h`, `UILayer.h`,
`UIScene_AbstractContainerMenu.h`, `UIScene_HUD.h`,
`UIComponent_Tooltips.h`, `UIComponent_TutorialPopup.h`,
`UIComponent_Chat.h`, `UIComponent_MenuBackground.h`,
`UIComponent_Panorama.h`.

### 11.2 Lifecycle callers

- `targets/app/desktop/main.cpp` - `Initialise`, `SetWindowSize`, `SetFullscreen`, `StartFrame`, `GetFramebufferSize`, `Tick`, `Present`, `Shutdown`, `ShouldClose`.
- `targets/app/common/Network/GameNetworkManager.cpp:1512, 1518, 1521` - secondary `StartFrame` / `Tick` / `Present` cycle for network-triggered rendering (loading screens while unblocking main loop).
- `targets/app/common/Game.cpp:349` - `Close` on quit.
- `targets/app/common/GameSettingsManager.cpp:261` - `UpdateGamma` when the gamma slider changes.

### 11.3 Real rendering work

- `targets/app/common/UI/UIController.cpp:965-1049`:
  - `StartFrame()` - begin a frame (secondary to the main game frame).
  - `Set_matrixDirty()` - invalidate the matrix shadow so the next draw re-uploads.
  - `Clear(GL_DEPTH_BUFFER_BIT)` - depth clear between game and UI layers.
- `targets/app/common/UI/UIScene.cpp:564-589` - CBuffStart/End/Call for cached UI geometry (Section 10.2).
- `targets/app/common/UI/Scenes/Help & Options/UIScene_SkinSelectMenu.cpp:529` - `StateSetStencil(GL_EQUAL, ref, mask, writemask)` for stencil-masked skin preview. This is the single most complex app/UI rendering call and deserves explicit preservation in `MaterialDesc::stencil_op`.

### 11.4 HUD / component renderers

These files use GL state macros (`glColor4f`, `glEnable`, `glDisable`,
`glPushMatrix`, etc.) rather than direct `PlatformRenderer` calls, so they
don't show up in the direct grep but they are heavy indirect users of the
interface through `gl_compat.h`:

- `UIComponent_HUD.cpp` / `IUIScene_HUD.cpp`
- `UIComponent_Chat.cpp`
- `UIComponent_Logo.cpp`
- `UIComponent_MenuBackground.cpp`
- `UIComponent_Panorama.cpp` (world-space panorama rendering on the main menu)
- `UIComponent_TutorialPopup.cpp`

All of these migrate as part of Phase 3 (GUI / overlay migration).

## 12. Dead API surface

Confirmed dead, eligible for deletion before Phase 2 touches the header:

- Enum value `VERTEX_TYPE_PF3_TF2_CB4_NB4_XW1_LIT` (IPlatformRenderer.h:12) - zero `DrawVertices` callers pass it.
- Enum value `PIXEL_SHADER_TYPE_FORCELOD` (IPlatformRenderer.h:20) - zero `DrawVertices` callers pass it. (`StateSetForceLOD` the method is still used; only the shader enum value is dead.)
- `TextureGetStats` (IPlatformRenderer.h:139) - stub with zero callers.
- `TextureGetTexture` (IPlatformRenderer.h:140) - stub returning `nullptr`, zero callers.
- `SaveTextureData` (IPlatformRenderer.h:130-132) - zero callers.
- `SaveTextureDataToMemory` (IPlatformRenderer.h:133-137) - zero callers.
- `DoScreenGrabOnNextPresent` (IPlatformRenderer.h:200) - zero callers.
- `TextureDynamicUpdateStart` / `TextureDynamicUpdateEnd` (IPlatformRenderer.h:121-122) - only call sites are commented out at Textures.cpp:1149, 1152.

Probably dead, verify before deletion:

- `BeginConditionalSurvey`, `EndConditionalSurvey`, `BeginConditionalRendering`, `EndConditionalRendering` (IPlatformRenderer.h:194-197) - occlusion-query contract with zero direct callers. The `occlusion_culling` meson option has multiple modes (frustum, BFS, hardware query) and the hardware-query mode would be the caller; confirm that mode is actually wired up before deleting.
- `CaptureThumbnail`, `CaptureScreen` (IPlatformRenderer.h:201-203) - `gameServices().captureSaveThumbnail()` routes elsewhere; confirm these are genuinely orphaned.

Not dead but wrong shape (reshape in Phase 1, do not delete):

- `LoadTextureData` overloads (IPlatformRenderer.h:123-129) - should be on an `ImageLoader`, not on `IRenderPath`.
- `D3DXIMAGE_INFO` (`targets/platform/PlatformTypes.h:41-44`) - legacy type leaking through the interface.

## 13. Migration risk callouts

### 13.1 Frustum state-reader is already solved by Camera

`Frustum.cpp:56-64` reads the current projection and modelview matrices via
`MatrixGet`, but the in-code comment explicitly says `Camera::prepare()`
already captures both matrices every frame. The GL backend stores a shadow
copy specifically to answer `MatrixGet`. Migration is a change of data
source, not a new capture pipeline. Cross-reference Section 5.1 and
Section 8.

### 13.2 Worker-thread chunk rebuilds cross the thread-transparent contract

Chunk workers call `CBuffClear`, `CBuffStart`, `DrawVertices` (via
Tesselator), and `CBuffEnd` from non-main threads with thread-local GL
state (Section 7). The new `create_mesh` / `update_mesh` / `destroy_mesh`
contract must be callable from any thread and must preserve the
`CBuffDeferredModeStart` / `End` atomic-group semantics. This is the
single biggest implementation risk for Phase 5. Cross-reference Sections
7.2 and 10.1.

### 13.3 App/UI subsystem is a larger migration target than initially scoped

28 .cpp files + headers under `targets/app/` reference the renderer. Most
are trivial resolution queries, but `UIController`, `UIScene`, and
`UIScene_SkinSelectMenu` touch real rendering state (command buffers,
stencil, depth clear). Phase 3 must plan for all of these, not just the
`targets/minecraft/client/gui/` directory. Cross-reference Section 11.

### 13.4 Fixed-function fog and lighting are first-class material/view state

`GameRenderer::setupFog` has seven distinct fog profiles selected from
gameplay state (blindness, clouds, underwater, lava, open air, bedrock fog,
fog dimension) and each one chooses between linear and exponential modes
with different start/end/density. The pass index (`i == -1` for sky fog,
`i == 0` for world fog, `i == 1` for item-in-hand) means the same view can
have multiple fog profiles depending on which pass is rendering. Phase 1's
`ViewDesc::fog` must either allow multiple fog profiles per view or allow
per-draw fog override. Cross-reference Section 9.

### 13.5 `Tesselator::end` encodes a runtime material branch

The Tesselator.cpp:106-144 block selects among five (layout, shader) pairs
based on two runtime booleans (`useCompactFormat360`,
`useProjectedTexturePixelShader`) and the primitive mode. The new
`MaterialDesc` contract must be able to express this branch as data, not
as a conditional at draw time. The three axes are:
- Vertex layout: compressed vs full.
- Shader: standard vs projected-texture.
- Primitive: triangle list / strip / fan / line list / line strip.

Cross-reference Section 3.

### 13.6 Viewport enum int-cast hides live values

`player->m_iScreenSection` is an `int` cast to `eViewportType` at
Minecraft.cpp:1634. All nine viewport values are live at runtime even
though only `FULLSCREEN` and `QUADRANT_*` appear as literal enum names at
`StateSetViewport` call sites. Static analysis and dead-code detection
will miss this. Phase 1's `ViewDesc::viewport_layout` must support all
nine values. Cross-reference Section 6.1.

### 13.7 `gl_compat.h` is the real migration surface

Direct `PlatformRenderer.` grep significantly understates the caller count
for matrix, state, fog, lighting, and command-buffer methods because
`gl_compat.h` routes dozens of legacy `gl*` calls through
`PlatformRenderer`. Section 2 disposition entries marked "via macro"
indicate methods where the majority of callers will not appear in a
direct grep. Phase 2 and Phase 3 migration must count `gl*` call sites in
the affected subsystem, not just `PlatformRenderer.*` call sites.
Cross-reference Section 1.1.

## 14. Next step

Phase 0 closes with this document. Phase 1 begins by writing `MeshDesc`,
`TextureDesc`, `MaterialDesc`, `ViewDesc`, and `FrameDesc` specs, using
the dispositions in Section 2 and the data in Sections 3, 4, 6, 7, 8, 9,
10 as input. Phase 1's validation gate: every row in Section 2 must map
unambiguously onto a field in one of the new Desc structs (or be listed
in Section 12 as dead).

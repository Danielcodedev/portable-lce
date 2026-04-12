$input a_position, a_texcoord0, a_color0, a_normal, a_texcoord1
$output v_uv0, v_uv1, v_color, v_fogFactor

#include <bgfx_shader.sh>

uniform vec4 u_baseColor;
uniform vec4 u_chunkOffset;    // xyz = chunk offset
uniform vec4 u_lightParams;    // x = lighting enabled, y = normalSign
uniform vec4 u_light0Dir;
uniform vec4 u_light1Dir;
uniform vec4 u_lightDiffuse;
uniform vec4 u_lightAmbient;
uniform vec4 u_fogParams;      // x = mode (0=off,1=linear,2=exp,3=exp2), y = start, z = end, w = density
uniform vec4 u_lmTransform;    // xy = scale, zw = offset
uniform vec4 u_globalLM;       // xy = global lightmap coords

void main()
{
    vec4 pos4 = vec4(a_position + u_chunkOffset.xyz, 1.0);
    vec4 eyePos = mul(u_modelView, pos4);
    gl_Position = mul(u_modelViewProj, pos4);

    v_uv0 = a_texcoord0;

    // Lightmap UV from vertex data or global fallback
    vec2 lm = (a_texcoord1.x <= -1.9) ? u_globalLM.xy : a_texcoord1;
    v_uv1 = (lm / 256.0) * u_lmTransform.xy + u_lmTransform.zw;

    // Color: zero vertex color = use base color uniform
    bool sentinel = (a_color0.r + a_color0.g + a_color0.b) < 0.004;
    // bgfx delivers color as RGBA normalized; the original GL had ABGR swizzle
    vec4 col = sentinel ? u_baseColor : a_color0;

    if (u_lightParams.x > 0.5) {
        vec3 n = normalize(mul(u_modelView, vec4(a_normal.xyz * 2.0 - 1.0, 0.0)).xyz) * u_lightParams.y;
        float d0 = max(dot(n, u_light0Dir.xyz), 0.0);
        float d1 = max(dot(n, u_light1Dir.xyz), 0.0);
        v_color = vec4(col.rgb * (u_lightAmbient.rgb + u_lightDiffuse.rgb * clamp(d0 + d1, 0.0, 1.0)), col.a);
    } else {
        v_color = col;
    }

    // Fog
    float eDist = length(eyePos.xyz);
    int fogMode = int(u_fogParams.x);
    if (fogMode == 1) {
        v_fogFactor = clamp((u_fogParams.z - eDist) / max(u_fogParams.z - u_fogParams.y, 0.0001), 0.0, 1.0);
    } else if (fogMode == 2) {
        v_fogFactor = clamp(exp(-u_fogParams.w * eDist), 0.0, 1.0);
    } else if (fogMode == 3) {
        float d = u_fogParams.w * eDist;
        v_fogFactor = clamp(exp(-d * d), 0.0, 1.0);
    } else {
        v_fogFactor = 1.0;
    }
}

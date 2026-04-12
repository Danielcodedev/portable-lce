$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);
uniform vec4 u_tintColor;
uniform vec4 u_params;

void main()
{
    vec4 texColor = texture2D(s_texColor, v_texcoord0);
    // u_params.x = 1.0 if textured, 0.0 if untextured
    vec4 color = mix(vec4(1.0), texColor, u_params.x);
    gl_FragColor = color * v_color0;
}

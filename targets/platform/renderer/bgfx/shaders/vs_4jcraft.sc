$input a_position, a_texcoord0, a_color0
$output v_color0, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_tintColor;

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_texcoord0 = a_texcoord0;
    // If vertex color is zero (sentinel), use u_tintColor instead
    v_color0 = a_color0.x + a_color0.y + a_color0.z > 0.001
        ? a_color0
        : u_tintColor;
}

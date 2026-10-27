$input a_position
$output v_color0, v_texcoord0
#include "bgfx_shader.sh"
SAMPLER2D(s_center,0);
SAMPLER2D(s_basis,1);
SAMPLER2D(s_color,2);
uniform vec4 u_counts;
void main() {
    vec2 uv=(vec2(mod(a_position.z,u_counts.z),floor(a_position.z/u_counts.z))+vec2(0.5,0.5))/u_counts.zw;
    vec4 center=texture2DLod(s_center,uv,0.0);
    vec4 basis=texture2DLod(s_basis,uv,0.0);
    v_color0=texture2DLod(s_color,uv,0.0);
    v_texcoord0=a_position.xy*3.0;
    gl_Position=vec4(center.xy+center.zw*v_texcoord0.x+basis.xy*v_texcoord0.y,0.5,1.0);
}

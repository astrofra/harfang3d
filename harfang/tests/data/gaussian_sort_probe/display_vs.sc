$input a_position
$output v_texcoord0
#include "bgfx_shader.sh"
void main() {
    gl_Position=vec4(a_position.xy,0.5,1.0);
    v_texcoord0=vec2(a_position.x,-a_position.y)*0.5+vec2(0.5,0.5);
}

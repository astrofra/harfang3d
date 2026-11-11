$input a_position
$output v_color0
#include "bgfx_shader.sh"
SAMPLER2D(s_atlas, 0);
void main() {
	vec2 cell = vec2(mod(a_position.z, 16.0), floor(a_position.z / 16.0));
	vec2 center = vec2(-1.0, 1.0) + (cell + vec2(0.5, 0.5)) * vec2(0.125, -0.125);
	gl_Position = vec4(center + a_position.xy * 0.0625, 0.5, 1.0);
	v_color0 = texture2DLod(s_atlas, (cell + vec2(0.5, 0.5)) / 16.0, 0.0);
}

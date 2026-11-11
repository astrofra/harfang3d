$input a_position, a_texcoord0
$output v_color0

#include "bgfx_shader.sh"

SAMPLER2D(s_atlas, 0);

void main() {
	gl_Position = vec4(a_position, 1.0);
	// Intentionally fetch in the vertex stage: fragment sampling would not
	// exercise the compute-to-vertex dependency required by the splat viewer.
	v_color0 = texture2DLod(s_atlas, a_texcoord0, 0.0);
}

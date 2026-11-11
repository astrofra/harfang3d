#include "bgfx_compute.sh"

BUFFER_RO(b_scratch, vec4, 0);
IMAGE2D_WR(u_atlas, rgba32f, 1);

NUM_THREADS(8, 8, 1)
void main() {
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	if (coord.x < 16 && coord.y < 16) {
		uint i = uint(coord.y * 16 + coord.x);
		uint scratch_index = (i * 17u + 3u) % 256u;
		vec4 value = b_scratch[scratch_index];
		float tag = float(64u + (scratch_index * 13u) % 256u);
		vec4 color = vec4(value.xyz / 255.0, 1.0);
		if (abs(value.w - tag) > 0.01)
			color = vec4(1.0, 0.0, 1.0, 1.0);
		imageStore(u_atlas, coord, color);
	}
}

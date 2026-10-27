#include "bgfx_compute.sh"

BUFFER_RO(b_input, vec4, 0);
BUFFER_RW(b_scratch, vec4, 1);
uniform vec4 u_seed;

NUM_THREADS(64, 1, 1)
void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i < 256u) {
		uint source = (i * 13u) % 256u;
		b_scratch[i] = b_input[source] + vec4(u_seed.x, u_seed.x * 2.0, u_seed.x * 3.0, 0.0);
	}
}

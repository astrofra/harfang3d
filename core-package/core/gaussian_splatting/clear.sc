#include "../shader/bgfx_compute.sh"

BUFFER_RW(b_keys, vec4, 0);
uniform vec4 u_counts; // padded count

NUM_THREADS(64, 1, 1)
void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i >= uint(u_counts.x))
		return;
	b_keys[i] = vec4(-1.0, float(i), 0.0, 0.0);
}

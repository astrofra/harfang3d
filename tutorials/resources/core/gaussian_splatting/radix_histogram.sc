#include "../shader/bgfx_compute.sh"

BUFFER_RO(b_keys, vec4, 0);
BUFFER_RW(b_hist, uvec4, 1);
uniform vec4 u_radix; // shift, padded count, block count

#define RADIX_BLOCK_SIZE 256
#define RADIX_THREADS 128

SHARED uint counts[16];

uint radixDigit(vec4 key) {
	uint sortKey = key.x < 0.0 ? 0xffffffffu : ~floatBitsToUint(key.x);
	return (sortKey >> uint(u_radix.x)) & 15u;
}

NUM_THREADS(RADIX_THREADS, 1, 1)
void main() {
	uint lane = gl_LocalInvocationID.x;
	if (lane < 16u)
		counts[lane] = 0u;
	barrier();

	uint block = gl_WorkGroupID.x;
	uint paddedCount = uint(u_radix.y);
	uint firstLocalIndex = lane;
	uint secondLocalIndex = lane + 128u;
	uint firstIndex = block * 256u + firstLocalIndex;
	uint secondIndex = block * 256u + secondLocalIndex;

	if (firstIndex < paddedCount)
		atomicAdd(counts[radixDigit(b_keys[firstIndex])], 1u);
	if (secondIndex < paddedCount)
		atomicAdd(counts[radixDigit(b_keys[secondIndex])], 1u);
	barrier();

	if (lane < 16u)
		b_hist[lane * uint(u_radix.z) + block] = uvec4(counts[lane], 0u, 0u, 0u);
}

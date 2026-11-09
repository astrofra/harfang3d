#include "../shader/bgfx_compute.sh"

BUFFER_RO(b_hist, uvec4, 0);
BUFFER_RW(b_offsets, uvec4, 1);
uniform vec4 u_radix; // shift, padded count, block count

#define RADIX_PREFIX_THREADS 128

SHARED uint sums[RADIX_PREFIX_THREADS];

void inclusiveScan(uint lane) {
	for (uint stride = 1u; stride < 128u; stride *= 2u) {
		uint value = 0u;
		if (lane >= stride)
			value = sums[lane - stride];
		barrier();
		if (lane >= stride)
			sums[lane] += value;
		barrier();
	}
}

NUM_THREADS(RADIX_PREFIX_THREADS, 1, 1)
void main() {
	uint bin = gl_WorkGroupID.x;
	uint lane = gl_LocalInvocationID.x;
	uint blockCount = uint(u_radix.z);
	uint chunk = (blockCount + 127u) / 128u;
	uint first = lane * chunk;
	uint last = min(first + chunk, blockCount);

	uint ownSum = 0u;
	for (uint block = first; block < last; ++block)
		ownSum += b_hist[bin * blockCount + block].x;

	uint earlierSum = 0u;
	for (uint index = lane; index < bin * blockCount; index += 128u)
		earlierSum += b_hist[index].x;

	sums[lane] = ownSum;
	barrier();
	inclusiveScan(lane);
	uint precedingSum = lane > 0u ? sums[lane - 1u] : 0u;

	// Reuse the 512 B scan storage for the earlier-bin inclusive scan.
	barrier();
	sums[lane] = earlierSum;
	barrier();
	inclusiveScan(lane);

	uint prefix = sums[RADIX_PREFIX_THREADS - 1] + precedingSum;
	for (uint block = first; block < last; ++block) {
		uint index = bin * blockCount + block;
		b_offsets[index] = uvec4(prefix, 0u, 0u, 0u);
		prefix += b_hist[index].x;
	}
}

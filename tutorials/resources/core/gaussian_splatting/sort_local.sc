#include "../shader/bgfx_compute.sh"

BUFFER_RO(b_in, vec4, 0);
BUFFER_RW(b_out, vec4, 1);
uniform vec4 u_sort; // stride, sequence length, padded count, initial flag

#define SORT_LOCAL_SIZE 256
#define SORT_LOCAL_THREADS 128

SHARED vec4 scratch[SORT_LOCAL_SIZE];

bool sortBefore(vec4 a, vec4 b) {
	// Far-to-near depth, then global instance/source index for a total order.
	return a.x > b.x || (a.x == b.x && a.y < b.y);
}

vec4 loadKey(uint index, uint paddedCount) {
	if (index < paddedCount)
		return b_in[index];
	return vec4(-1.0, float(index), 0.0, 0.0);
}

vec4 sortRead(uint localIndex, uint globalIndex, uint stride, uint sequenceLength) {
	vec4 a = scratch[localIndex];
	vec4 b = scratch[localIndex ^ stride];
	bool before = sortBefore(a, b);
	bool first = ((globalIndex & sequenceLength) == 0u) == ((globalIndex & stride) == 0u);
	return before == first ? a : b;
}

NUM_THREADS(SORT_LOCAL_THREADS, 1, 1)
void main() {
	uint localIndex = gl_LocalInvocationID.x;
	uint globalIndex = gl_WorkGroupID.x * SORT_LOCAL_SIZE + localIndex;
	uint secondLocalIndex = localIndex + SORT_LOCAL_THREADS;
	uint secondGlobalIndex = globalIndex + SORT_LOCAL_THREADS;
	uint paddedCount = uint(u_sort.z);

	scratch[localIndex] = loadKey(globalIndex, paddedCount);
	scratch[secondLocalIndex] = loadKey(secondGlobalIndex, paddedCount);
	barrier();

	if (u_sort.w > 0.0) {
		uint lastSequence = paddedCount < SORT_LOCAL_SIZE ? paddedCount : SORT_LOCAL_SIZE;
		for (uint sequenceLength = 2u; sequenceLength <= lastSequence; sequenceLength *= 2u) {
			for (uint stride = sequenceLength / 2u; stride > 0u; stride /= 2u) {
				vec4 firstValue = sortRead(localIndex, globalIndex, stride, sequenceLength);
				vec4 secondValue = sortRead(secondLocalIndex, secondGlobalIndex, stride, sequenceLength);
				barrier();
				scratch[localIndex] = firstValue;
				scratch[secondLocalIndex] = secondValue;
				barrier();
			}
		}
	} else {
		uint sequenceLength = uint(u_sort.y);
		for (uint stride = uint(u_sort.x); stride > 0u; stride /= 2u) {
			vec4 firstValue = sortRead(localIndex, globalIndex, stride, sequenceLength);
			vec4 secondValue = sortRead(secondLocalIndex, secondGlobalIndex, stride, sequenceLength);
			barrier();
			scratch[localIndex] = firstValue;
			scratch[secondLocalIndex] = secondValue;
			barrier();
		}
	}

	if (globalIndex < paddedCount)
		b_out[globalIndex] = scratch[localIndex];
	if (secondGlobalIndex < paddedCount)
		b_out[secondGlobalIndex] = scratch[secondLocalIndex];
}

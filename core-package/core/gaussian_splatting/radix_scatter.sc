#include "../shader/bgfx_compute.sh"

BUFFER_RO(b_keys, vec4, 0);
BUFFER_RO(b_offsets, uvec4, 1);
BUFFER_RW(b_out, vec4, 2);
uniform vec4 u_radix; // shift, padded count, block count

#define RADIX_BLOCK_SIZE 256
#define RADIX_THREADS 128

SHARED uint masks[16 * 8];

uint radixDigit(vec4 key) {
	uint sortKey = key.x < 0.0 ? 0xffffffffu : ~floatBitsToUint(key.x);
	return (sortKey >> uint(u_radix.x)) & 15u;
}

uint radixPopcount(uint value) {
	value = value - ((value >> 1u) & 0x55555555u);
	value = (value & 0x33333333u) + ((value >> 2u) & 0x33333333u);
	value = (value + (value >> 4u)) & 0x0f0f0f0fu;
	return (value * 0x01010101u) >> 24u;
}

uint radixRank(uint digit, uint localIndex) {
	uint wordIndex = localIndex / 32u;
	uint rank = 0u;
	for (uint word = 0u; word < wordIndex; ++word)
		rank += radixPopcount(masks[digit * 8u + word]);

	uint bitIndex = localIndex & 31u;
	uint precedingMask = 0u;
	if (bitIndex != 0u)
		precedingMask = (1u << bitIndex) - 1u;
	rank += radixPopcount(masks[digit * 8u + wordIndex] & precedingMask);
	return rank;
}

NUM_THREADS(RADIX_THREADS, 1, 1)
void main() {
	uint lane = gl_LocalInvocationID.x;
	masks[lane] = 0u;
	barrier();

	uint block = gl_WorkGroupID.x;
	uint paddedCount = uint(u_radix.y);
	uint firstLocalIndex = lane;
	uint secondLocalIndex = lane + 128u;
	uint firstIndex = block * 256u + firstLocalIndex;
	uint secondIndex = block * 256u + secondLocalIndex;
	bool firstValid = firstIndex < paddedCount;
	bool secondValid = secondIndex < paddedCount;
	vec4 firstKey = vec4_splat(0.0);
	vec4 secondKey = vec4_splat(0.0);
	uint firstDigit = 0u;
	uint secondDigit = 0u;

	if (firstValid) {
		firstKey = b_keys[firstIndex];
		firstDigit = radixDigit(firstKey);
		atomicOr(masks[firstDigit * 8u + firstLocalIndex / 32u], 1u << (firstLocalIndex & 31u));
	}
	if (secondValid) {
		secondKey = b_keys[secondIndex];
		secondDigit = radixDigit(secondKey);
		atomicOr(masks[secondDigit * 8u + secondLocalIndex / 32u], 1u << (secondLocalIndex & 31u));
	}
	barrier();

	if (firstValid) {
		uint rank = radixRank(firstDigit, firstLocalIndex);
		uint destination = b_offsets[firstDigit * uint(u_radix.z) + block].x + rank;
		b_out[destination] = firstKey;
	}
	if (secondValid) {
		uint rank = radixRank(secondDigit, secondLocalIndex);
		uint destination = b_offsets[secondDigit * uint(u_radix.z) + block].x + rank;
		b_out[destination] = secondKey;
	}
}

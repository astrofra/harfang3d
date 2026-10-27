#include "bgfx_compute.sh"
BUFFER_RO(b_in, vec4, 0);
BUFFER_RW(b_out, vec4, 1);
uniform vec4 u_sort; // stride, sequence length, padded count
NUM_THREADS(64, 1, 1)
void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= uint(u_sort.z)) return;
    uint j = i ^ uint(u_sort.x);
    vec4 a = b_in[i], b = b_in[j];
    // Total order: far-to-near, then original index ascending for equal depth.
    bool before = a.x > b.x || (a.x == b.x && a.y < b.y);
    bool first = ((i & uint(u_sort.y)) == 0u) == ((i & uint(u_sort.x)) == 0u);
    b_out[i] = before == first ? a : b;
}

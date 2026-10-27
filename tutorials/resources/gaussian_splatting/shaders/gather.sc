#include "bgfx_compute.sh"
BUFFER_RO(b_projected, vec4, 0);
BUFFER_RO(b_keys, vec4, 1);
IMAGE2D_WR(u_center, rgba32f, 2);
IMAGE2D_WR(u_basis, rgba32f, 3);
IMAGE2D_WR(u_color, rgba32f, 4);
uniform vec4 u_counts;
NUM_THREADS(8, 8, 1)
void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    if (coord.x >= int(u_counts.z) || coord.y >= int(u_counts.w)) return;
    uint slot = uint(coord.y)*uint(u_counts.z)+uint(coord.x);
    vec4 center=vec4(0.0,0.0,0.0,0.0), basis=center, color=center;
    if (slot < uint(u_counts.x)) {
        vec4 key = b_keys[slot];
        if (key.x > 0.0) {
            uint i = uint(key.y);
            center = b_projected[i*3u]; basis = b_projected[i*3u+1u]; color = b_projected[i*3u+2u];
        }
    }
    imageStore(u_center,coord,center);
    imageStore(u_basis,coord,basis);
    imageStore(u_color,coord,color);
}

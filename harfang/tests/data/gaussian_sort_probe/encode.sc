#include "bgfx_compute.sh"
BUFFER_RO(b_keys,vec4,0);
IMAGE2D_WR(u_atlas,rgba32f,1);
NUM_THREADS(8,8,1)
void main() {
    ivec2 p=ivec2(gl_GlobalInvocationID.xy);
    uint i=uint(p.y*512+p.x);
    uint id=uint(b_keys[i].y);
    imageStore(u_atlas,p,vec4(float(id & 255u),float((id>>8u)&255u),float((id>>16u)&255u),255.0)/255.0);
}

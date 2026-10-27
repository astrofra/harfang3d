#include "bgfx_compute.sh"
BUFFER_RO(b_source, vec4, 0);
BUFFER_RW(b_projected, vec4, 1);
BUFFER_RW(b_keys, vec4, 2);
uniform vec4 u_counts; // actual, padded, atlas width, atlas height
uniform vec4 u_eye;
uniform vec4 u_right;
uniform vec4 u_up;
uniform vec4 u_forward;
uniform vec4 u_screen; // width, height, focal pixels, near

vec3 rotateQ(vec4 q, vec3 v) {
    return v + 2.0 * cross(q.yzw, cross(q.yzw, v) + q.x * v);
}
vec3 cameraVector(vec3 v) { return vec3(dot(v,u_right.xyz), dot(v,u_up.xyz), dot(v,u_forward.xyz)); }
NUM_THREADS(64, 1, 1)
void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= uint(u_counts.y)) return;
    b_keys[i] = vec4(-1.0, float(i), 0.0, 0.0);
    if (i >= uint(u_counts.x)) return;
    b_projected[i*3u] = vec4(0.0,0.0,0.0,0.0);
    b_projected[i*3u+1u] = vec4(0.0,0.0,0.0,0.0);
    b_projected[i*3u+2u] = vec4(0.0,0.0,0.0,0.0);
    vec4 p = b_source[i*4u];
    vec3 v = cameraVector(p.xyz-u_eye.xyz);
    if (v.z < u_screen.w || p.w < 1.0/255.0) return;
    vec3 scale = b_source[i*4u+1u].xyz;
    vec4 q = b_source[i*4u+2u];
    vec3 a = cameraVector(rotateQ(q,vec3(scale.x,0.0,0.0)));
    vec3 b = cameraVector(rotateQ(q,vec3(0.0,scale.y,0.0)));
    vec3 c = cameraVector(rotateQ(q,vec3(0.0,0.0,scale.z)));
    float focal = u_screen.z/v.z;
    // Jacobian of perspective projection, applied to the covariance factors.
    vec3 jx = vec3(focal,0.0,-focal*v.x/v.z);
    vec3 jy = vec3(0.0,focal,-focal*v.y/v.z);
    vec3 x = vec3(dot(jx,a),dot(jx,b),dot(jx,c));
    vec3 y = vec3(dot(jy,a),dot(jy,b),dot(jy,c));
    float xx = dot(x,x)+0.3, yy = dot(y,y)+0.3, xy = dot(x,y);
    float mid = 0.5*(xx+yy);
    float delta = sqrt(max(0.0,0.25*(xx-yy)*(xx-yy)+xy*xy));
    float l1 = max(0.3,mid+delta), l2 = max(0.3,mid-delta);
    vec2 axis = abs(xy)>0.00001 ? normalize(vec2(xy,l1-xx)) : (xx>=yy ? vec2(1.0,0.0) : vec2(0.0,1.0));
    vec2 center = v.xy*focal;
    // Bounded footprint limits pathological near-camera overdraw (tutorial quality tradeoff).
    vec2 major = axis*min(sqrt(l1),256.0);
    vec2 minor = vec2(-axis.y,axis.x)*min(sqrt(l2),256.0);
    vec2 radius = 3.0*(abs(major)+abs(minor));
    if (abs(center.x)>u_screen.x*0.5+radius.x || abs(center.y)>u_screen.y*0.5+radius.y) return;
    vec2 ndc = vec2(2.0/u_screen.x,2.0/u_screen.y);
    b_projected[i*3u] = vec4(center*ndc,major*ndc);
    b_projected[i*3u+1u] = vec4(minor*ndc,0.0,0.0);
    b_projected[i*3u+2u] = vec4(b_source[i*4u+3u].xyz,p.w);
    b_keys[i] = vec4(v.z,float(i),0.0,0.0);
}

$input v_color0, v_texcoord0
#include "bgfx_shader.sh"
void main() {
    float r2=dot(v_texcoord0,v_texcoord0);
    if (r2>9.0) discard;
    float alpha=min(0.99,v_color0.a*exp(-0.5*r2));
    if (alpha<1.0/255.0) discard;
    gl_FragColor=vec4(v_color0.rgb*alpha,alpha);
}

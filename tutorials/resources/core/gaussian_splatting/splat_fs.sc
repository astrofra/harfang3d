$input v_color0, v_texcoord0

#include "../shader/bgfx_shader.sh"

void main() {
	float radius2 = dot(v_texcoord0, v_texcoord0);
	if (radius2 > 9.0)
		discard;
	float alpha = min(0.99, v_color0.a * exp(-0.5 * radius2));
	if (alpha < 1.0 / 255.0)
		discard;
	// The blend state is ONE/INV_SRC_ALPHA: emit premultiplied color.
	gl_FragColor = vec4(v_color0.rgb * alpha, alpha);
}

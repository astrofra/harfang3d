#include "../shader/bgfx_compute.sh"

BUFFER_RO(b_source, vec4, 0);
BUFFER_RW(b_projected, vec4, 1);
BUFFER_RW(b_keys, vec4, 2);
BUFFER_RO(b_higher_sh, vec4, 3);

uniform vec4 u_counts; // local count, global offset, padded count, stored non-DC count
uniform vec4 u_eye;
uniform vec4 u_view0;
uniform vec4 u_view1;
uniform vec4 u_view2;
uniform vec4 u_proj0;
uniform vec4 u_proj1;
uniform vec4 u_proj2;
uniform vec4 u_proj3;
uniform vec4 u_screen; // viewport width, height
uniform vec4 u_depth_range; // NDC minimum, maximum
uniform vec4 u_world0;
uniform vec4 u_world1;
uniform vec4 u_world2;
uniform vec4 u_sh0; // normalized instance rotation columns
uniform vec4 u_sh1;
uniform vec4 u_sh2;
uniform vec4 u_params; // degree, non-DC count, Postshot antialiasing, pixel radius cap
uniform vec4 u_instance; // opacity

vec3 rotateQ(vec4 q, vec3 v) {
	return v + 2.0 * cross(q.yzw, cross(q.yzw, v) + q.x * v);
}

vec3 transformPoint(vec4 r0, vec4 r1, vec4 r2, vec3 p) {
	return vec3(dot(r0, vec4(p, 1.0)), dot(r1, vec4(p, 1.0)), dot(r2, vec4(p, 1.0)));
}

vec3 transformVector(vec4 r0, vec4 r1, vec4 r2, vec3 v) {
	return vec3(dot(r0.xyz, v), dot(r1.xyz, v), dot(r2.xyz, v));
}

float shBasis(uint index, vec3 d) {
	const float c0 = 0.28209479177387814;
	const float c1 = 0.4886025119029199;
	if (index == 0u)
		return c0;
	if (index == 1u)
		return -c1 * d.y;
	if (index == 2u)
		return c1 * d.z;
	if (index == 3u)
		return -c1 * d.x;
	float x = d.x, y = d.y, z = d.z;
	if (index == 4u)
		return 1.0925484305920792 * x * y;
	if (index == 5u)
		return -1.0925484305920792 * y * z;
	if (index == 6u)
		return 0.31539156525252005 * (2.0 * z * z - x * x - y * y);
	if (index == 7u)
		return -1.0925484305920792 * x * z;
	if (index == 8u)
		return 0.5462742152960396 * (x * x - y * y);
	if (index == 9u)
		return -0.5900435899266435 * y * (3.0 * x * x - y * y);
	if (index == 10u)
		return 2.890611442640554 * x * y * z;
	if (index == 11u)
		return -0.4570457994644658 * y * (4.0 * z * z - x * x - y * y);
	if (index == 12u)
		return 0.3731763325901154 * z * (2.0 * z * z - 3.0 * x * x - 3.0 * y * y);
	if (index == 13u)
		return -0.4570457994644658 * x * (4.0 * z * z - x * x - y * y);
	if (index == 14u)
		return 1.445305721320277 * z * (x * x - y * y);
	return -0.5900435899266435 * x * (x * x - 3.0 * y * y);
}

NUM_THREADS(64, 1, 1)
void main() {
	uint local = gl_GlobalInvocationID.x;
	if (local >= uint(u_counts.x))
		return;
	uint index = uint(u_counts.y) + local;
	b_keys[index] = vec4(-1.0, float(index), 0.0, 0.0);
	b_projected[index * 3u] = vec4_splat(0.0);
	b_projected[index * 3u + 1u] = vec4_splat(0.0);
	b_projected[index * 3u + 2u] = vec4_splat(0.0);

	vec4 source = b_source[local * 4u];
	if (source.w <= 0.0 || u_instance.x <= 0.0)
		return;
	vec3 mean = transformPoint(u_world0, u_world1, u_world2, source.xyz);
	vec3 view = transformPoint(u_view0, u_view1, u_view2, mean);
	if (view.z <= 0.0)
		return;

	vec4 clip = vec4(
		dot(u_proj0, vec4(view, 1.0)),
		dot(u_proj1, vec4(view, 1.0)),
		dot(u_proj2, vec4(view, 1.0)),
		dot(u_proj3, vec4(view, 1.0)));
	if (clip.w <= 0.0)
		return;
	vec3 ndc = clip.xyz / clip.w;
	if (ndc.z < u_depth_range.x || ndc.z > u_depth_range.y)
		return;

	// Transform the three covariance factors through the instance and view linear maps.
	vec3 scale = b_source[local * 4u + 1u].xyz;
	vec4 rotation = b_source[local * 4u + 2u];
	vec3 a = transformVector(u_view0, u_view1, u_view2,
		transformVector(u_world0, u_world1, u_world2, rotateQ(rotation, vec3(scale.x, 0.0, 0.0))));
	vec3 b = transformVector(u_view0, u_view1, u_view2,
		transformVector(u_world0, u_world1, u_world2, rotateQ(rotation, vec3(0.0, scale.y, 0.0))));
	vec3 c = transformVector(u_view0, u_view1, u_view2,
		transformVector(u_world0, u_world1, u_world2, rotateQ(rotation, vec3(0.0, 0.0, scale.z))));

	// Jacobian of the full standard/off-axis perspective projection, in pixel units.
	vec3 jx = vec3(u_proj0.x / view.z, 0.0, (u_proj0.z - ndc.x) / view.z) * (u_screen.x * 0.5);
	vec3 jy = vec3(0.0, u_proj1.y / view.z, (u_proj1.z - ndc.y) / view.z) * (u_screen.y * 0.5);
	vec3 x = vec3(dot(jx, a), dot(jx, b), dot(jx, c));
	vec3 y = vec3(dot(jy, a), dot(jy, b), dot(jy, c));
	float xx0 = dot(x, x), yy0 = dot(y, y), xy0 = dot(x, y);
	float xx = xx0 + 0.3, yy = yy0 + 0.3, xy = xy0;
	float opacity = source.w * u_instance.x;
	if (u_params.z > 0.5) {
		float unfiltered = max(0.0, xx0 * yy0 - xy0 * xy0);
		float filtered = max(1e-20, xx * yy - xy * xy);
		opacity *= sqrt(max(0.0, unfiltered) / filtered);
	}
	if (opacity <= 0.0)
		return;

	float middle = 0.5 * (xx + yy);
	float delta = sqrt(max(0.0, 0.25 * (xx - yy) * (xx - yy) + xy * xy));
	float l1 = max(0.3, middle + delta), l2 = max(0.3, middle - delta);
	vec2 axis = abs(xy) > 0.00001 ? normalize(vec2(xy, l1 - xx)) : (xx >= yy ? vec2(1.0, 0.0) : vec2(0.0, 1.0));
	vec2 major = axis * min(sqrt(l1), u_params.w);
	vec2 minor = vec2(-axis.y, axis.x) * min(sqrt(l2), u_params.w);
	vec2 centerPixels = ndc.xy * u_screen.xy * 0.5;
	vec2 radius = 3.0 * (abs(major) + abs(minor));
	if (abs(centerPixels.x) > u_screen.x * 0.5 + radius.x || abs(centerPixels.y) > u_screen.y * 0.5 + radius.y)
		return;

	// Evaluate directional color only after all visibility culling has passed.
	vec3 cameraVector = mean - u_eye.xyz;
	float cameraLength2 = dot(cameraVector, cameraVector);
	vec3 direction = cameraLength2 > 1e-20 ? cameraVector * (1.0 / sqrt(cameraLength2)) : vec3(0.0, 0.0, 1.0);
	direction = vec3(dot(u_sh0.xyz, direction), dot(u_sh1.xyz, direction), dot(u_sh2.xyz, direction));
	vec3 color = vec3_splat(0.5) + b_source[local * 4u + 3u].xyz * shBasis(0u, direction);
	uint nonDC = uint(u_params.y);
	uint storedNonDC = uint(u_counts.w);
	for (uint k = 0u; k < 15u; ++k) {
		if (k >= nonDC)
			break;
		vec4 coefficient = b_higher_sh[local * storedNonDC + k];
		color += coefficient.xyz * shBasis(k + 1u, direction);
	}
	color = max(color, vec3_splat(0.0));

	vec2 ndcScale = vec2(2.0 / u_screen.x, 2.0 / u_screen.y);
	b_projected[index * 3u] = vec4(ndc.xy, major * ndcScale);
	b_projected[index * 3u + 1u] = vec4(minor * ndcScale, ndc.z, 0.0);
	b_projected[index * 3u + 2u] = vec4(color, opacity);
	b_keys[index] = vec4(view.z, float(index), 0.0, 0.0);
}

// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/trackball_controller.h"

#include "foundation/matrix3.h"
#include "platform/input_system.h"

#include <cmath>

namespace hg {

static Vec3 TrackballSphere(float x, float y, float width, float height) {
	const float radius = Max(1.f, Min(width, height) * 0.5f);
	x = (x - width * 0.5f) / radius;
	y = (y - height * 0.5f) / radius;
	const float d = x * x + y * y;
	return d > 1.f ? Normalize(Vec3(x, y, 0.f)) : Vec3(x, y, -Sqrt(1.f - d));
}

TrackballController::TrackballController() : TrackballController(Vec3::Zero, 3.4f) {}

TrackballController::TrackballController(const Vec3 &target, float distance, const Quaternion &orientation)
	: target(target), orientation(Normalize(orientation)), distance(distance),
	  initial_target(this->target), initial_orientation(this->orientation), initial_distance(this->distance) {}

void TrackballController::Reset() {
	target = initial_target;
	orientation = initial_orientation;
	distance = Clamp(initial_distance, min_distance, max_distance);
}

void TrackballController::Update(bool orbit, bool pan, float x, float y, float dx, float dy, float wheel, float width, float height, float fov) {
	if (width <= 0.f || height <= 0.f)
		return;

	orientation = Normalize(orientation);
	if (orbit) {
		const Vec3 a = TrackballSphere(x - dx, y - dy, width, height);
		const Vec3 b = TrackballSphere(x, y, width, height);
		const float dot = Clamp(Dot(a, b), -1.f, 1.f);
		if (dot > -0.999999f) {
			// Inverse object rotation: move the camera around the target.
			const Vec3 axis = Cross(b, a);
			const Quaternion delta = Normalize(Quaternion(axis.x, axis.y, axis.z, 1.f + dot));
			orientation = Normalize(orientation * delta);
		}
	}

	distance = Clamp(distance * std::exp(-wheel * dolly_speed), min_distance, max_distance);
	if (pan && fov > 0.f && fov < Pi) {
		const Vec3 right = Rotate(orientation, Vec3::Right), up = Rotate(orientation, Vec3::Up);
		const float scale = 2.f * distance * Tan(fov * 0.5f) / Max(height, 1.f);
		target -= (right * dx + up * dy) * scale;
	}
}

void TrackballController::Update(const Mouse &mouse, float width, float height, float fov, bool drag_valid) {
	const bool drag = drag_valid && !mouse.Pressed(MB_0) && !mouse.Pressed(MB_1) && !mouse.Pressed(MB_2);
	Update(drag && mouse.Down(MB_0), drag && (mouse.Down(MB_1) || mouse.Down(MB_2)), float(mouse.X()), float(mouse.Y()),
		float(mouse.DtX()), float(mouse.DtY()), mouse.WheelDelta(), width, height, fov);
}

Mat4 TrackballController::ComputeWorld() const {
	const Mat3 rotation = ToMatrix3(Normalize(orientation));
	return TransformationMat4(target - GetZ(rotation) * distance, rotation);
}

} // namespace hg

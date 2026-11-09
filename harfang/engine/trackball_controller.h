// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/matrix4.h"
#include "foundation/quaternion.h"
#include "foundation/vector3.h"

namespace hg {

struct Mouse;

/// Orbit a camera around a target using a quaternion arcball, exponential dolly and screen-space pan.
struct TrackballController {
	TrackballController();
	/// Capture the reset pose. distance must be positive; limits are applied by Update and Reset.
	TrackballController(const Vec3 &target, float distance, const Quaternion &orientation = Quaternion::Identity);

	Vec3 target;
	Quaternion orientation;
	float distance;
	float min_distance{0.15f}, max_distance{50.f}, dolly_speed{0.12f};

	/// Restore the pose captured at construction, keeping the current distance limits and dolly speed.
	void Reset();

	/// Pointer coordinates/deltas and viewport dimensions are in the same units, with Y pointing down.
	/// fov is the vertical field of view in radians; distance limits must be positive and ordered.
	/// Ignore updates for empty viewports. An antipodal arcball drag leaves the orientation unchanged.
	void Update(bool orbit, bool pan, float x, float y, float dx, float dy, float wheel, float width, float height, float fov = Pi / 3.f);

	/// Left button orbits; right/middle button pans. First-press deltas are suppressed.
	/// Set drag_valid=false when reacquiring focus; dolly remains enabled. Gate calls on window focus in the application.
	void Update(const Mouse &mouse, float width, float height, float fov = Pi / 3.f, bool drag_valid = true);

	/// Return the camera world transform (not its inverse/view matrix), including roll.
	Mat4 ComputeWorld() const;

private:
	Vec3 initial_target;
	Quaternion initial_orientation;
	float initial_distance;
};

} // namespace hg

// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/matrix3.h"
#include "foundation/matrix4.h"
#include "foundation/vector3.h"

namespace hg {

namespace tau_compat {

// Harfang-owned transform bridge for the future Tau item adapter seam.
class NodeMotionAdapter {
public:
	NodeMotionAdapter() = default;

	void Reset(const Mat4 &world = Mat4::Identity);
	void CaptureSourceWorld(const Mat4 &world);
	void WriteSolvedWorld(const Mat4 &world);

	const Mat4 &GetWorld() const { return world; }
	const Mat4 &GetPreviousWorld() const { return previous_world; }
	const Mat3 &GetOrientationMatrix() const { return orientation; }
	const Vec3 &GetPosition() const { return position; }
	const Vec3 &GetRotation() const { return rotation; }
	const Vec3 &GetScale() const { return scale; }

	const Mat4 &GetInverseWorld() const;

private:
	void SetCurrentWorld(const Mat4 &value);

	Mat4 world{Mat4::Identity};
	Mat4 previous_world{Mat4::Identity};
	Mat3 orientation{Mat3::Identity};
	Vec3 position{Vec3::Zero};
	Vec3 rotation{Vec3::Zero};
	Vec3 scale{Vec3::One};

	mutable Mat4 inverse_world{Mat4::Identity};
	mutable bool inverse_dirty{false};
};

} // namespace tau_compat

} // namespace hg

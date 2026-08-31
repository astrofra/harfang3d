// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "tau_harfang_runtime.h"

namespace hg {
namespace tau_compat {

void NodeMotionAdapter::Reset(const Mat4 &value) {
	previous_world = value;
	SetCurrentWorld(value);
}

void NodeMotionAdapter::CaptureSourceWorld(const Mat4 &value) {
	previous_world = world;
	SetCurrentWorld(value);
}

void NodeMotionAdapter::WriteSolvedWorld(const Mat4 &value) {
	previous_world = world;
	SetCurrentWorld(value);
}

const Mat4 &NodeMotionAdapter::GetInverseWorld() const {
	if (inverse_dirty) {
		inverse_world = InverseFast(world);
		inverse_dirty = false;
	}
	return inverse_world;
}

void NodeMotionAdapter::SetCurrentWorld(const Mat4 &value) {
	world = value;
	Decompose(world, &position, &orientation, &scale);
	rotation = ToEuler(orientation);
	inverse_dirty = true;
}

} // namespace tau_compat
} // namespace hg

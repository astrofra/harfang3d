// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#if HG_ENABLE_BULLET3_SCENE_PHYSICS
#include "engine/scene_bullet3_physics.h"
#endif
#if HG_ENABLE_TAU_SCENE_PHYSICS
#include "engine/scene_tau_physics.h"
#endif

namespace hg {

/// Return the display name of the scene physics backend selected at build time.
const char *GetScenePhysicsBackendName();

#if HG_ENABLE_BULLET3_SCENE_PHYSICS
class ScenePhysics : public SceneBullet3Physics {
public:
	using SceneBullet3Physics::SceneBullet3Physics;

	void SetPreTickCallback(const std::function<void(ScenePhysics &, hg::time_ns t)> &cbk) {
		if (cbk)
			SceneBullet3Physics::SetPreTickCallback([this, cbk](SceneBullet3Physics &, hg::time_ns t) { cbk(*this, t); });
		else
			SceneBullet3Physics::SetPreTickCallback({});
	}
};
#elif HG_ENABLE_TAU_SCENE_PHYSICS
class ScenePhysics : public SceneTauPhysics {
public:
	using SceneTauPhysics::SceneTauPhysics;

	void SetPreTickCallback(const std::function<void(ScenePhysics &, hg::time_ns t)> &cbk) {
		if (cbk)
			SceneTauPhysics::SetPreTickCallback([this, cbk](SceneTauPhysics &, hg::time_ns t) { cbk(*this, t); });
		else
			SceneTauPhysics::SetPreTickCallback({});
	}
};

// Keep source code using the historical physics entry point buildable with Tau.
using SceneBullet3Physics = ScenePhysics;
#endif

} // namespace hg

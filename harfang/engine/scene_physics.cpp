// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene.h"
#include "engine/scene_physics.h"

namespace hg {

const char *GetScenePhysicsBackendName() {
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
	return "Bullet Physics";
#elif HG_ENABLE_TAU_SCENE_PHYSICS
	return "Tau";
#else
	return "None";
#endif
}

} // namespace hg

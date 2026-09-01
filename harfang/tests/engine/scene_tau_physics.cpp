// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/scene.h"
#include "engine/scene_physics.h"
#include "engine/scene_tau_physics.h"

using namespace hg;

namespace {

void TestPreTickCallback() {
	Scene scene;
	const Node cube = CreatePhysicCube(scene, Vec3::One, TranslationMat4(Vec3(0.f, 2.f, 0.f)), InvalidModelRef, {}, 1.f);

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);

	int callback_count = 0;
	time_ns callback_dt = 0;
	SceneTauPhysics *callback_physics = nullptr;
	physics.SetPreTickCallback([&](SceneTauPhysics &ph, time_ns dt) {
		++callback_count;
		callback_dt = dt;
		callback_physics = &ph;
		ph.NodeSetLinearVelocity(cube, Vec3(0.f, 10.f, 0.f));
	});

	physics.StepSimulation(time_from_ms(32), time_from_ms(16), 8);
	TEST_CHECK(callback_count == 2);
	TEST_CHECK(callback_physics == &physics);
	TEST_CHECK(callback_dt >= time_from_ms(15) && callback_dt <= time_from_ms(17));
	TEST_CHECK(physics.NodeGetLinearVelocity(cube).y > 9.f);

	physics.SetPreTickCallback({});
	physics.StepSimulation(time_from_ms(16), time_from_ms(16), 8);
	TEST_CHECK(callback_count == 2);
}

void TestScenePhysicsPreTickAdapter() {
	ScenePhysics physics;
	int callback_count = 0;
	ScenePhysics *callback_physics = nullptr;
	physics.SetPreTickCallback([&](ScenePhysics &ph, time_ns) {
		++callback_count;
		callback_physics = &ph;
	});

	physics.StepSimulation(time_from_ms(16), time_from_ms(16), 8);
	TEST_CHECK(callback_count == 1);
	TEST_CHECK(callback_physics == &physics);
}

} // namespace

void test_scene_tau_physics() {
	TestPreTickCallback();
	TestScenePhysicsPreTickAdapter();
}

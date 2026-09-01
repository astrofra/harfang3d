// HARFANG(R) Copyright (C) 2026 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "engine/collision_geometry.h"
#include "engine/file_format.h"
#include "engine/scene.h"
#include "engine/scene_physics.h"
#include "engine/scene_tau_physics.h"

#include "foundation/data.h"
#include "foundation/data_rw_interface.h"
#include "foundation/file.h"

#include "../utils.h"

using namespace hg;

namespace {

Node CreateTauRaycastPrimitive(Scene &scene, CollisionType type, const Vec3 &position, float radius = 0.5f, float height = 1.f) {
	Node node = scene.CreateNode();
	node.SetTransform(scene.CreateTransform(TranslationMat4(position)));
	auto rigid_body = scene.CreateRigidBody();
	rigid_body.SetType(RBT_Dynamic);
	node.SetRigidBody(rigid_body);
	auto collision = scene.CreateCollision();
	collision.SetType(type);
	collision.SetMass(0.f);
	if (type == CT_Cube)
		collision.SetSize(Vec3::One);
	else {
		collision.SetRadius(radius);
		collision.SetHeight(height);
	}
	node.SetCollision(0, collision);
	return node;
}

Node CreateTauContactPrimitive(
	Scene &scene, CollisionType type, const Vec3 &position, RigidBodyType body_type, float mass, float radius = 0.5f, float height = 1.f) {
	Node node = scene.CreateNode();
	node.SetTransform(scene.CreateTransform(TranslationMat4(position)));
	auto rigid_body = scene.CreateRigidBody();
	rigid_body.SetType(body_type);
	node.SetRigidBody(rigid_body);
	auto collision = scene.CreateCollision();
	collision.SetType(type);
	collision.SetMass(mass);
	if (type == CT_Cube)
		collision.SetSize(Vec3::One);
	else {
		collision.SetRadius(radius);
		if (type == CT_Capsule)
			collision.SetHeight(height);
	}
	node.SetCollision(0, collision);
	return node;
}

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

void TestFixedStepAccumulation() {
	SceneTauPhysics physics;
	int callback_count = 0;
	time_ns callback_dt = 0;
	physics.SetPreTickCallback([&](SceneTauPhysics &, time_ns dt) {
		++callback_count;
		callback_dt = dt;
	});

	// A partial fixed step is accumulated without advancing the simulation.
	physics.StepSimulation(time_from_ms(8), time_from_ms(16), 4);
	TEST_CHECK(callback_count == 0);
	physics.StepSimulation(time_from_ms(8), time_from_ms(16), 4);
	TEST_CHECK(callback_count == 1);
	TEST_CHECK(callback_dt == time_from_ms(16));

	// Exact multiples execute fixed-duration callbacks.
	physics.StepSimulation(time_from_ms(48), time_from_ms(16), 4);
	TEST_CHECK(callback_count == 4);
	TEST_CHECK(callback_dt == time_from_ms(16));

	// Full fixed steps beyond the cap are dropped, while the fractional
	// remainder is retained. The following 16 ms frame therefore runs once.
	physics.StepSimulation(time_from_ms(80), time_from_ms(16), 2);
	TEST_CHECK(callback_count == 6);
	physics.StepSimulation(time_from_ms(16), time_from_ms(16), 2);
	TEST_CHECK(callback_count == 7);

	// Bullet's maxSubSteps == 0 behavior is one variable-duration step.
	physics.StepSimulation(time_from_ms(32), time_from_ms(16), 0);
	TEST_CHECK(callback_count == 8);
	TEST_CHECK(callback_dt == time_from_ms(32));

	// Clearing the world also clears a partial fixed-step remainder.
	SceneTauPhysics cleared_physics;
	int cleared_callback_count = 0;
	cleared_physics.SetPreTickCallback([&](SceneTauPhysics &, time_ns) { ++cleared_callback_count; });
	cleared_physics.StepSimulation(time_from_ms(8), time_from_ms(16), 4);
	cleared_physics.ClearNodes();
	cleared_physics.StepSimulation(time_from_ms(8), time_from_ms(16), 4);
	TEST_CHECK(cleared_callback_count == 0);
	cleared_physics.StepSimulation(time_from_ms(8), time_from_ms(16), 4);
	TEST_CHECK(cleared_callback_count == 1);
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

void StepTauWorld(SceneTauPhysics &physics, int step_count) {
	for (int step = 0; step < step_count; ++step)
		physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
}

void StepTauSceneWorld(Scene &scene, SceneTauPhysics &physics, int step_count) {
	for (int step = 0; step < step_count; ++step) {
		scene.StorePreviousWorldMatrices();
		scene.ReadyWorldMatrices();
		physics.SyncTransformsFromScene(scene);
		physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
		physics.SyncTransformsToScene(scene);
		scene.ComputeWorldMatrices();
		scene.FixupPreviousWorldMatrices();
	}
}

void TestActiveBodyKeepsPublishedTransformWithoutSubstep() {
	Scene scene;
	const Node body = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 3.f, 0.f), RBT_Dynamic, 1.f);
	scene.ReadyWorldMatrices();

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	StepTauSceneWorld(scene, physics, 1);
	const float stepped_y = GetT(body.GetWorld()).y;
	TEST_CHECK(stepped_y < 3.f);

	// Eight milliseconds do not complete the 16 ms fixed step. Scene matrix
	// invalidation still occurs, so the active body must republish its current
	// physics pose even though no integration marked it dirty this frame.
	scene.StorePreviousWorldMatrices();
	scene.ReadyWorldMatrices();
	physics.SyncTransformsFromScene(scene);
	physics.StepSimulation(time_from_ms(8), time_from_ms(16), 1);
	physics.SyncTransformsToScene(scene);
	scene.ComputeWorldMatrices();
	scene.FixupPreviousWorldMatrices();
	const float unstepped_y = GetT(body.GetWorld()).y;
	TEST_CHECK_(Abs(unstepped_y - stepped_y) < 0.0001f, "active cube jumped from y=%.6f to y=%.6f", stepped_y, unstepped_y);
}

void TestSleepingBodyKeepsPublishedTransform() {
	Scene scene;
	const Node body = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 3.f, 0.f), RBT_Dynamic, 1.f);
	CreatePhysicCube(scene, Vec3(10.f, 1.f, 10.f), TranslationMat4(Vec3(0.f, -0.5f, 0.f)), InvalidModelRef, {}, 0.f);
	scene.ReadyWorldMatrices();

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	StepTauSceneWorld(scene, physics, 360);
	TEST_CHECK(tau_internal::IsNodeSleeping(physics, body.ref));

	const float settled_y = GetT(body.GetWorld()).y;
	TEST_CHECK_(settled_y > 0.45f && settled_y < 0.55f, "expected sleeping cube center near y=0.5, got %.6f", settled_y);

	// A sleeping body is no longer dirty, so Tau does not publish its cached
	// world matrix on this frame. The persistent Transform must nevertheless
	// keep the settled pose when the scene rebuilds all world matrices.
	StepTauSceneWorld(scene, physics, 1);
	const float sleeping_y = GetT(body.GetWorld()).y;
	TEST_CHECK_(Abs(sleeping_y - settled_y) < 0.0001f, "sleeping cube jumped from y=%.6f to y=%.6f", settled_y, sleeping_y);
}

void TestSleepingBodyWakeAndTrackedContacts() {
	Scene scene;
	const Node body = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 0.5f, 0.f), RBT_Dynamic, 1.f);
	const Node floor = CreatePhysicCube(
		scene, Vec3(10.f, 1.f, 10.f), TranslationMat4(Vec3(0.f, -0.5f, 0.f)), InvalidModelRef, {}, 0.f);
	scene.ReadyWorldMatrices();

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	physics.NodeStartTrackingCollisionEvents(body, CETM_EventAndContacts);
	StepTauWorld(physics, 240);
	TEST_CHECK(tau_internal::IsNodeSleeping(physics, body.ref));
	TEST_CHECK(Len2(physics.NodeGetLinearVelocity(body)) == 0.f);
	TEST_CHECK(Len2(physics.NodeGetAngularVelocity(body)) == 0.f);

	// A tracked resting pair remains observable while its solver island sleeps.
	physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
	NodePairContacts contacts;
	physics.CollectCollisionEvents(scene, contacts);
	TEST_CHECK(!GetNodeRefPairContacts(body.ref, floor.ref, contacts).empty());
	TEST_CHECK(tau_internal::IsNodeSleeping(physics, body.ref));

	physics.NodeWake(body);
	TEST_CHECK(!tau_internal::IsNodeSleeping(physics, body.ref));
	physics.NodeAddImpulse(body, Vec3(1.f, 0.f, 0.f));
	TEST_CHECK(!tau_internal::IsNodeSleeping(physics, body.ref));
	physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
	TEST_CHECK(physics.NodeGetLinearVelocity(body).x > 0.5f);
}

void TestSleepingIslandWakePropagation() {
	Scene scene;
	const Node bottom = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 0.5f, 0.f), RBT_Dynamic, 1.f);
	const Node top = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 1.5f, 0.f), RBT_Dynamic, 1.f);
	CreatePhysicCube(scene, Vec3(10.f, 1.f, 10.f), TranslationMat4(Vec3(0.f, -0.5f, 0.f)), InvalidModelRef, {}, 0.f);
	scene.ReadyWorldMatrices();

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	StepTauWorld(physics, 360);
	TEST_CHECK(tau_internal::IsNodeSleeping(physics, bottom.ref));
	TEST_CHECK(tau_internal::IsNodeSleeping(physics, top.ref));

	// The API mutation wakes the complete persisted contact island before the
	// next fixed step, not only the addressed rigid body.
	physics.NodeAddImpulse(bottom, Vec3(1.f, 0.f, 0.f));
	TEST_CHECK(!tau_internal::IsNodeSleeping(physics, bottom.ref));
	TEST_CHECK(!tau_internal::IsNodeSleeping(physics, top.ref));
}

void TestSleepingBodyWakesOnImpact() {
	Scene scene;
	const Node target = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 0.5f, 0.f), RBT_Dynamic, 1.f);
	CreatePhysicCube(scene, Vec3(10.f, 1.f, 10.f), TranslationMat4(Vec3(0.f, -0.5f, 0.f)), InvalidModelRef, {}, 0.f);
	scene.ReadyWorldMatrices();

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	StepTauWorld(physics, 240);
	TEST_CHECK(tau_internal::IsNodeSleeping(physics, target.ref));

	const Node projectile = CreateTauContactPrimitive(scene, CT_Cube, Vec3(-2.f, 0.5f, 0.f), RBT_Dynamic, 1.f);
	physics.NodeCreatePhysicsFromAssets(projectile);
	physics.NodeSetDeactivation(projectile, false);
	physics.NodeSetLinearVelocity(projectile, Vec3(8.f, 0.f, 0.f));
	for (int step = 0; step < 30 && tau_internal::IsNodeSleeping(physics, target.ref); ++step)
		physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
	TEST_CHECK(!tau_internal::IsNodeSleeping(physics, target.ref));
	TEST_CHECK(physics.NodeGetLinearVelocity(target).x > 0.f);
}

void TestSleepingBodyWakesWithDynamicSupportCohort() {
	Scene scene;
	std::vector<Node> graph_bodies;
	graph_bodies.reserve(65);

	// Fill the first 63 slots of a connected sleep graph with immobile bodies.
	// The lower support then becomes slot 64 and the supported body slot 65,
	// placing them on opposite sides of the bounded 64-body cohort boundary.
	for (int i = 0; i < 63; ++i)
		graph_bodies.push_back(CreateTauContactPrimitive(scene, CT_Cube, Vec3(100.f + float(i) * 2.f, 0.5f, 0.f), RBT_Dynamic, 1.f));
	const Node support = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 0.5f, 0.f), RBT_Dynamic, 1.f);
	const Node supported = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 1.5f, 0.f), RBT_Dynamic, 1.f);
	graph_bodies.push_back(support);
	graph_bodies.push_back(supported);
	CreatePhysicCube(scene, Vec3(300.f, 1.f, 10.f), TranslationMat4(Vec3(0.f, -0.5f, 0.f)), InvalidModelRef, {}, 0.f);
	scene.ReadyWorldMatrices();

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	// Prevent gravity from perturbing the test-authored support pose.
	physics.NodeSetLinearFactor(support, Vec3(1.f, 0.f, 0.f));
	for (size_t i = 0; i + 1 < graph_bodies.size(); ++i)
		physics.Add6DofConstraint(graph_bodies[i], graph_bodies[i + 1], Mat4::Identity, Mat4::Identity);
	for (size_t i = 0; i < 63; ++i) {
		physics.NodeSetLinearFactor(graph_bodies[i], Vec3::Zero);
		physics.NodeSetAngularFactor(graph_bodies[i], Vec3::Zero);
	}
	StepTauWorld(physics, 240);
	TEST_ASSERT(tau_internal::IsNodeSleeping(physics, support.ref));
	TEST_ASSERT(tau_internal::IsNodeSleeping(physics, supported.ref));
	TEST_CHECK(tau_internal::GetNodeSleepIslandId(physics, support.ref) != tau_internal::GetNodeSleepIslandId(physics, supported.ref));
	TEST_ASSERT(tau_internal::HasNodeSleepingSupportSnapshot(physics, supported.ref));

	// Emulate the bounded wake performed by an impact on the lower cohort. The
	// supported body is deliberately in the next cohort and initially remains
	// asleep. Move the support in 3 mm increments: no individual change reaches
	// the 2 cm tolerance, but cumulative displacement must wake the dependency.
	for (int step = 0; step < 10 && tau_internal::IsNodeSleeping(physics, supported.ref); ++step) {
		tau_internal::TransformNodeSleepCohortForTest(
			physics, support.ref, Vec3(0.003f, 0.f, 0.f), Quaternion::Identity);
		physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
	}
	TEST_ASSERT(!tau_internal::IsNodeSleeping(physics, support.ref));
	TEST_CHECK(!tau_internal::IsNodeSleeping(physics, supported.ref));

	// Re-settle at the displaced pose and validate the rotational half of the
	// same invariant independently of translation.
	StepTauWorld(physics, 240);
	TEST_ASSERT(tau_internal::IsNodeSleeping(physics, support.ref));
	TEST_ASSERT(tau_internal::IsNodeSleeping(physics, supported.ref));
	TEST_ASSERT(tau_internal::HasNodeSleepingSupportSnapshot(physics, supported.ref));
	tau_internal::TransformNodeSleepCohortForTest(
		physics, support.ref, Vec3::Zero, QuaternionFromAxisAngle(Deg(3.f), Vec3::Up));
	TEST_ASSERT(!tau_internal::IsNodeSleeping(physics, support.ref));
	TEST_ASSERT(tau_internal::IsNodeSleeping(physics, supported.ref));
	physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
	TEST_CHECK(!tau_internal::IsNodeSleeping(physics, supported.ref));
}

void TestDeactivationAndMovingSupportWake() {
	{
		Scene scene;
		const Node body = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 0.5f, 0.f), RBT_Dynamic, 1.f);
		CreatePhysicCube(scene, Vec3(10.f, 1.f, 10.f), TranslationMat4(Vec3(0.f, -0.5f, 0.f)), InvalidModelRef, {}, 0.f);
		scene.ReadyWorldMatrices();
		SceneTauPhysics physics;
		physics.SceneCreatePhysicsFromAssets(scene);
		physics.NodeSetDeactivation(body, false);
		StepTauWorld(physics, 240);
		TEST_CHECK(!physics.NodeGetDeactivation(body));
		TEST_CHECK(!tau_internal::IsNodeSleeping(physics, body.ref));
		physics.NodeSetDeactivation(body, true);
		StepTauWorld(physics, 240);
		TEST_CHECK(tau_internal::IsNodeSleeping(physics, body.ref));
	}

	{
		Scene scene;
		const Node support = CreateTauContactPrimitive(scene, CT_Cube, Vec3::Zero, RBT_Kinematic, 0.f);
		const Node body = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.f, 1.f, 0.f), RBT_Dynamic, 1.f);
		scene.ReadyWorldMatrices();
		SceneTauPhysics physics;
		physics.SceneCreatePhysicsFromAssets(scene);
		StepTauWorld(physics, 240);
		TEST_CHECK(tau_internal::IsNodeSleeping(physics, body.ref));

		support.GetTransform().SetPos(Vec3(0.f, -0.25f, 0.f));
		physics.SyncTransformsFromScene(scene);
		TEST_CHECK(!tau_internal::IsNodeSleeping(physics, body.ref));
		physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
		TEST_CHECK(physics.NodeGetLinearVelocity(body).y < 0.f);
	}
}

void TestRaycastVariousCollisionShapes() {
	Scene scene;
	const Node sphere = CreateTauRaycastPrimitive(scene, CT_Sphere, Vec3(2.f, 1.f, 2.5f));
	const Node cube = CreateTauRaycastPrimitive(scene, CT_Cube, Vec3(0.f, 1.f, 2.5f));
	const Node capsule = CreateTauRaycastPrimitive(scene, CT_Capsule, Vec3(-2.f, 1.f, 2.5f));
	const Node cone = CreateTauRaycastPrimitive(scene, CT_Cone, Vec3(-4.f, 1.f, 2.5f));
	const Node cylinder = CreateTauRaycastPrimitive(scene, CT_Cylinder, Vec3(-6.f, 1.f, 2.5f));

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	TEST_CHECK(physics.NodeHasBody(sphere));
	TEST_CHECK(physics.NodeHasBody(cube));
	TEST_CHECK(physics.NodeHasBody(capsule));
	TEST_CHECK(physics.NodeHasBody(cone));
	TEST_CHECK(physics.NodeHasBody(cylinder));

	struct ExpectedHit {
		Node node;
		float x;
		float z;
	};
	const ExpectedHit expected[] = {
		{sphere, 2.f, 2.0669873f}, {cube, 0.f, 2.f}, {capsule, -2.f, 2.f}, {cone, -4.f, 2.0802786f}, {cylinder, -6.f, 2.f}};
	for (const auto &item : expected) {
		const RaycastOut hit = physics.RaycastFirstHit(scene, Vec3(item.x, 0.75f, -5.f), Vec3(item.x, 0.75f, 10.f));
		TEST_CHECK(hit.node == item.node);
		TEST_CHECK(Abs(hit.P.x - item.x) < 0.0001f);
		TEST_CHECK(Abs(hit.P.y - 0.75f) < 0.0001f);
		TEST_CHECK_(Abs(hit.P.z - item.z) < 0.001f, "shape x %.3f: expected z %.6f, got %.6f", item.x, item.z, hit.P.z);
		TEST_CHECK_(Abs(hit.t - (item.z + 5.f)) < 0.001f, "shape x %.3f: expected t %.6f, got %.6f", item.x, item.z + 5.f, hit.t);
		TEST_CHECK(hit.N.z < -0.8f);
	}

	const RaycastOut miss = physics.RaycastFirstHit(scene, Vec3(4.f, 0.75f, -5.f), Vec3(4.f, 0.75f, 10.f));
	TEST_CHECK(!miss.node.IsValid());
}

void TestCapsuleCollisionPairs() {
	Scene scene;
	const Node capsule_sphere = CreateTauContactPrimitive(scene, CT_Capsule, Vec3::Zero, RBT_Dynamic, 1.f);
	const Node sphere = CreateTauContactPrimitive(scene, CT_Sphere, Vec3(0.8f, 0.f, 0.f), RBT_Static, 0.f);

	// Create the cube first to exercise the reversed cube/capsule dispatch order.
	const Node cube = CreateTauContactPrimitive(scene, CT_Cube, Vec3(10.8f, 0.f, 0.f), RBT_Static, 0.f);
	const Node capsule_cube = CreateTauContactPrimitive(scene, CT_Capsule, Vec3(10.f, 0.f, 0.f), RBT_Dynamic, 1.f);

	const Node capsule_a = CreateTauContactPrimitive(scene, CT_Capsule, Vec3(20.f, 0.f, 0.f), RBT_Dynamic, 1.f);
	const Node capsule_b = CreateTauContactPrimitive(scene, CT_Capsule, Vec3(20.8f, 0.f, 0.f), RBT_Static, 0.f);

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	const Node dynamic_capsules[] = {capsule_sphere, capsule_cube, capsule_a};
	for (const Node &capsule : dynamic_capsules) {
		TEST_CHECK(physics.NodeHasBody(capsule));
		physics.NodeStartTrackingCollisionEvents(capsule, CETM_EventAndContacts);
		physics.NodeAddImpulse(capsule, Vec3(0.f, 0.f, 1.f));
		TEST_CHECK(physics.NodeGetLinearVelocity(capsule).z > 0.9f);
		physics.NodeSetLinearVelocity(capsule, Vec3::Zero);
	}

	physics.StepSimulation(time_from_ms(1), time_from_ms(1), 1);
	NodePairContacts contacts;
	physics.CollectCollisionEvents(scene, contacts);

	struct ExpectedPair {
		Node capsule;
		Node other;
	};
	const ExpectedPair expected[] = {{capsule_sphere, sphere}, {capsule_cube, cube}, {capsule_a, capsule_b}};
	for (const auto &pair : expected) {
		const auto pair_contacts = GetNodeRefPairContacts(pair.capsule.ref, pair.other.ref, contacts);
		TEST_CHECK_(pair_contacts.size() == 1, "expected one capsule contact, got %zu", pair_contacts.size());
		if (!pair_contacts.empty()) {
			TEST_CHECK(AlmostEqual(Len(pair_contacts[0].N), 1.f, 0.0001f));
			TEST_CHECK(pair_contacts[0].d <= 0.f);
		}
	}
}

void TestCapsuleSettlesOnCuboid() {
	Scene scene;
	const Node capsule = CreateTauContactPrimitive(scene, CT_Capsule, Vec3(0.f, 3.f, 0.f), RBT_Dynamic, 1.f);
	CreatePhysicCube(scene, Vec3(10.f, 1.f, 10.f), TranslationMat4(Vec3(0.f, -0.5f, 0.f)), InvalidModelRef, {}, 0.f);
	scene.ReadyWorldMatrices();

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	physics.NodeStartTrackingCollisionEvents(capsule, CETM_EventAndContacts);
	size_t contact_steps = 0;
	for (int step = 0; step < 240; ++step) {
		physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
		NodePairContacts contacts;
		physics.CollectCollisionEvents(scene, contacts);
		if (!GetNodeRefsInContact(capsule.ref, contacts).empty())
			++contact_steps;
	}
	physics.SyncTransformsToScene(scene);

	const float settled_y = GetT(capsule.GetTransform().GetWorld()).y;
	TEST_CHECK_(settled_y > 0.9f && settled_y < 1.1f, "expected capsule center near y=1, got %.6f", settled_y);
	TEST_CHECK(Abs(physics.NodeGetLinearVelocity(capsule).y) < 0.1f);
	TEST_CHECK(contact_steps > 100);
}

void TestCuboidManifoldCacheLifecycle() {
	Scene scene;
	const Node dynamic = CreateTauContactPrimitive(scene, CT_Cube, Vec3::Zero, RBT_Dynamic, 1.f);
	const Node fixed = CreateTauContactPrimitive(scene, CT_Cube, Vec3(0.8f, 0.f, 0.f), RBT_Static, 0.f);

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	physics.NodeStartTrackingCollisionEvents(dynamic, CETM_EventAndContacts);
	physics.StepSimulation(time_from_ms(1), time_from_ms(1), 1);
	NodePairContacts contacts;
	physics.CollectCollisionEvents(scene, contacts);
	TEST_CHECK(!GetNodeRefPairContacts(dynamic.ref, fixed.ref, contacts).empty());

	// Move away long enough to prune the persistent cuboid manifold, then
	// recreate the same cache key. This exercises hash-index removal and reuse.
	physics.NodeSetLinearVelocity(dynamic, Vec3(-100.f, 0.f, 0.f));
	for (int step = 0; step < 6; ++step)
		physics.StepSimulation(time_from_ms(16), time_from_ms(16), 1);
	physics.CollectCollisionEvents(scene, contacts);
	TEST_CHECK(GetNodeRefPairContacts(dynamic.ref, fixed.ref, contacts).empty());

	physics.NodeTeleport(dynamic, Mat4::Identity);
	physics.NodeSetLinearVelocity(dynamic, Vec3::Zero);
	physics.StepSimulation(time_from_ms(1), time_from_ms(1), 1);
	physics.CollectCollisionEvents(scene, contacts);
	TEST_CHECK(!GetNodeRefPairContacts(dynamic.ref, fixed.ref, contacts).empty());

	// Node removal and recreation must clear and rebuild both manifold stores.
	physics.NodeDestroyPhysics(fixed);
	physics.StepSimulation(time_from_ms(1), time_from_ms(1), 1);
	physics.CollectCollisionEvents(scene, contacts);
	TEST_CHECK(GetNodeRefPairContacts(dynamic.ref, fixed.ref, contacts).empty());
	physics.NodeCreatePhysicsFromAssets(fixed);
	physics.NodeTeleport(dynamic, Mat4::Identity);
	physics.StepSimulation(time_from_ms(1), time_from_ms(1), 1);
	physics.CollectCollisionEvents(scene, contacts);
	TEST_CHECK(!GetNodeRefPairContacts(dynamic.ref, fixed.ref, contacts).empty());
}

void TestRaycastAllHitsAreStableAndBounded() {
	Scene scene;
	const Node near_node = CreatePhysicCube(scene, Vec3::One, TranslationMat4(Vec3(0.f, 0.f, -2.f)), InvalidModelRef, {}, 0.f);
	const Node middle_node = CreatePhysicCube(scene, Vec3::One, Mat4::Identity, InvalidModelRef, {}, 0.f);
	const Node far_node = CreatePhysicCube(scene, Vec3::One, TranslationMat4(Vec3(0.f, 0.f, 2.f)), InvalidModelRef, {}, 0.f);

	SceneTauPhysics physics;
	physics.SceneCreatePhysicsFromAssets(scene);
	const auto hits = physics.RaycastAllHits(scene, Vec3(0.f, 0.f, -5.f), Vec3(0.f, 0.f, 5.f));
	TEST_CHECK(hits.size() == 3);
	if (hits.size() == 3) {
		TEST_CHECK(hits[0].node == near_node);
		TEST_CHECK(hits[1].node == middle_node);
		TEST_CHECK(hits[2].node == far_node);
		TEST_CHECK(hits[0].t < hits[1].t && hits[1].t < hits[2].t);
	}

	const auto bounded_hits = physics.RaycastAllHits(scene, Vec3(0.f, 0.f, -5.f), Vec3(0.f, 0.f, -3.f));
	TEST_CHECK(bounded_hits.empty());
	const auto zero_length_hits = physics.RaycastAllHits(scene, Vec3::Zero, Vec3::Zero);
	TEST_CHECK(zero_length_hits.empty());
}

void TestMeshColliderRaycast() {
	CollisionGeometry source;
	source.triangles = {
		{Vec3(-2.f, 0.f, -1.f), Vec3(2.f, 0.f, 1.f), Vec3(2.f, 0.f, -1.f)},
		{Vec3(-2.f, 0.f, -1.f), Vec3(-2.f, 0.f, 1.f), Vec3(2.f, 0.f, 1.f)},
	};

	Data serialized;
	TEST_ASSERT(SaveCollisionGeometry(g_data_writer, DataWriteHandle(serialized), source));
	serialized.Rewind();
	CollisionGeometry round_trip;
	TEST_ASSERT(LoadCollisionGeometry(g_data_reader, DataReadHandle(serialized), round_trip));
	TEST_CHECK(round_trip.triangles.size() == source.triangles.size());
	TEST_CHECK(round_trip.bounds.mn == Vec3(-2.f, 0.f, -1.f));
	TEST_CHECK(round_trip.bounds.mx == Vec3(2.f, 0.f, 1.f));
	TEST_CHECK(ValidateBVH(round_trip.triangle_bvh, round_trip.triangles.size()));
	TEST_CHECK(ValidateBVH(round_trip.boundary_bvh, round_trip.boundary_edges.size()));
	TEST_CHECK(round_trip.boundary_edges.size() == 4);

	// Version 0 contained triangles only. Keep loading it by building the same
	// reusable acceleration structures at runtime for backward compatibility.
	Data legacy_serialized;
	const DataWriteHandle legacy_write(legacy_serialized);
	TEST_ASSERT(Write(g_data_writer, legacy_write, HarfangMagic));
	TEST_ASSERT(Write(g_data_writer, legacy_write, CollisionGeometryMarker));
	TEST_ASSERT(Write(g_data_writer, legacy_write, uint32_t(0)));
	TEST_ASSERT(Write(g_data_writer, legacy_write, uint32_t(source.triangles.size())));
	TEST_ASSERT(g_data_writer.write(legacy_write, source.triangles.data(), source.triangles.size() * sizeof(CollisionTriangle)) ==
		source.triangles.size() * sizeof(CollisionTriangle));
	legacy_serialized.Rewind();
	CollisionGeometry legacy_round_trip;
	TEST_ASSERT(LoadCollisionGeometry(g_data_reader, DataReadHandle(legacy_serialized), legacy_round_trip));
	TEST_CHECK(ValidateBVH(legacy_round_trip.triangle_bvh, legacy_round_trip.triangles.size()));

	const std::string temporary = test::CreateTempFilepath();
	Unlink(temporary.c_str());
	const std::string logical_resource = temporary + ".physics";
	const std::string cooked_resource = logical_resource + "_triangles";
	TEST_ASSERT(SaveCollisionGeometryToFile(cooked_resource.c_str(), source));

	Scene scene;
	Node mesh = scene.CreateNode();
	mesh.SetTransform(scene.CreateTransform(TranslationMat4(Vec3(2.f, 0.f, 3.f))));
	auto rigid_body = scene.CreateRigidBody();
	rigid_body.SetType(RBT_Kinematic);
	mesh.SetRigidBody(rigid_body);
	auto collision = scene.CreateCollision();
	collision.SetType(CT_Mesh);
	collision.SetCollisionResource(logical_resource);
	collision.SetMass(0.f);
	mesh.SetCollision(0, collision);

	SceneTauPhysics physics;
	physics.NodeCreatePhysicsFromFile(mesh);
	TEST_ASSERT(physics.NodeHasBody(mesh));

	const RaycastOut hit = physics.RaycastFirstHit(scene, Vec3(2.f, 1.f, 3.f), Vec3(2.f, -1.f, 3.f));
	TEST_CHECK(hit.node == mesh);
	TEST_CHECK(Abs(hit.P.y) < 0.0001f);
	TEST_CHECK(Abs(hit.t - 1.f) < 0.0001f);
	TEST_CHECK(hit.N.y > 0.99f);

	const RaycastOut miss = physics.RaycastFirstHit(scene, Vec3(5.f, 1.f, 3.f), Vec3(5.f, -1.f, 3.f));
	TEST_CHECK(!miss.node.IsValid());
	const RaycastOut open_boundary = physics.RaycastFirstHit(scene, Vec3(4.f, 1.f, 3.f), Vec3(4.f, -1.f, 3.f));
	TEST_CHECK(!open_boundary.node.IsValid());

	// A kinematic body's scene transform is authoritative. After synchronization,
	// the mesh's long X axis is rotated onto Z and the former miss becomes a hit.
	const Vec3 rotated_probe(2.f, 1.f, 4.5f);
	TEST_CHECK(!physics.RaycastFirstHit(scene, rotated_probe, Vec3(rotated_probe.x, -1.f, rotated_probe.z)).node.IsValid());
	mesh.GetTransform().SetRot(Vec3(0.f, Deg(90.f), 0.f));
	physics.SyncTransformsFromScene(scene);
	const RaycastOut rotated_hit = physics.RaycastFirstHit(scene, rotated_probe, Vec3(rotated_probe.x, -1.f, rotated_probe.z));
	TEST_CHECK(rotated_hit.node == mesh);
	TEST_CHECK(Abs(rotated_hit.P.y) < 0.0001f);
	TEST_CHECK(rotated_hit.N.y > 0.99f);

	physics.NodeDestroyPhysics(mesh);
	TEST_CHECK(physics.GarbageCollectResources() == 1);
	Unlink(cooked_resource.c_str());
}

} // namespace

void test_scene_tau_physics() {
	TestPreTickCallback();
	TestFixedStepAccumulation();
	TestScenePhysicsPreTickAdapter();
	TestActiveBodyKeepsPublishedTransformWithoutSubstep();
	TestSleepingBodyKeepsPublishedTransform();
	TestSleepingBodyWakeAndTrackedContacts();
	TestSleepingIslandWakePropagation();
	TestSleepingBodyWakesOnImpact();
	TestSleepingBodyWakesWithDynamicSupportCohort();
	TestDeactivationAndMovingSupportWake();
	TestRaycastVariousCollisionShapes();
	TestCapsuleCollisionPairs();
	TestCapsuleSettlesOnCuboid();
	TestCuboidManifoldCacheLifecycle();
	TestRaycastAllHitsAreStableAndBounded();
	TestMeshColliderRaycast();
}

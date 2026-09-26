#define TEST_NO_MAIN
#include "acutest.h"
#undef near
#undef far
#include "engine/forward_pipeline.h"
#include "engine/scene.h"
#include "engine/scene_systems.h"
#include "foundation/data.h"
#include "foundation/file_rw_interface.h"
#include <cmath>

using namespace hg;

static Node AnimNode(Scene &scene, const char *name = "bone") {
	auto node = scene.CreateNode(name);
	node.SetTransform(scene.CreateTransform());
	return node;
}

static SceneAnimRef PositionClip(Scene &scene, Node node, const char *name, float from, float to, int duration_ms = 1000) {
	Anim anim;
	anim.t_end = time_from_ms(duration_ms);
	anim.vec3_tracks.push_back({"Position", {}});
	SetKey(anim.vec3_tracks.back(), time_ns(0), Vec3(from, 0, 0));
	SetKey(anim.vec3_tracks.back(), anim.t_end, Vec3(to, 0, 0));
	SceneAnim clip;
	clip.name = name;
	clip.t_end = anim.t_end;
	clip.node_anims.push_back({node.ref, scene.AddAnim(std::move(anim))});
	return scene.AddSceneAnim(std::move(clip));
}

static void CheckX(Node node, float expected) {
	const auto actual = node.GetTransform().GetPos().x;
	TEST_CHECK(std::abs(actual - expected) < 0.0001f);
	TEST_MSG("Expected position x = %f, got %f", expected, actual);
}

static void TestLoopCombinations() {
	for (auto a_mode : {ALM_Once, ALM_Loop})
		for (auto b_mode : {ALM_Once, ALM_Loop}) {
			Scene scene;
			auto node = AnimNode(scene);
			const auto a = PositionClip(scene, node, "a", 0, 10);
			const auto b = PositionClip(scene, node, "b", 10, 20, 500);
			auto player = scene.CreateAnimPlayer({a, b});
			TEST_ASSERT(player.IsValid());
			TEST_CHECK(player.SetCrossFadeDuration(time_from_ms(500)));
			const auto ar = player.Play("a", a_mode);
			scene.Update(time_from_ms(750));
			const auto br = player.Play("b", b_mode);
			TEST_CHECK(player.IsTransitioning());
			scene.Update(time_from_ms(250));
			CheckX(node, a_mode == ALM_Loop ? 7.5f : 12.5f); // A at wrap/end, B at its midpoint; weight 1/2.
			TEST_CHECK(scene.IsPlaying(ar) == (a_mode == ALM_Loop));
			TEST_CHECK(scene.IsPlaying(br));
			scene.Update(time_from_ms(250));
			CheckX(node, b_mode == ALM_Loop ? 10.f : 20.f);
			TEST_CHECK(!player.IsTransitioning());
			TEST_CHECK(!scene.IsPlaying(ar));
			TEST_CHECK(scene.IsPlaying(br) == (b_mode == ALM_Loop));
		}
}

static void TestEndpointsAndInterruptions() {
	Scene scene;
	auto node = AnimNode(scene);
	const auto a = PositionClip(scene, node, "a", 0, 0, 50);
	const auto b = PositionClip(scene, node, "b", 10, 10, 100);
	const auto c = PositionClip(scene, node, "c", 20, 20);
	auto player = scene.CreateAnimPlayer({a, b, c});
	player.SetCrossFadeDuration(time_from_ms(200));
	const auto ar = player.Play("a");
	scene.Update(time_from_ms(50));
	TEST_CHECK(!scene.IsPlaying(ar));
	const auto br = player.Play("b");
	scene.Update(time_from_ms(100));
	CheckX(node, 5);
	TEST_CHECK(!scene.IsPlaying(br) && player.IsTransitioning());
	const auto cr = player.Play("c", ALM_Loop);
	scene.Update(0);
	CheckX(node, 5); // Interrupt from the mixed pose, not B's pose.
	scene.Update(time_from_ms(100));
	CheckX(node, 12.5f);
	TEST_CHECK(player.Play("c", ALM_Loop) == cr);
	TEST_CHECK(player.Play("missing") == InvalidScenePlayAnimRef);
	TEST_CHECK(!player.SetCrossFadeDuration(-1));
	TEST_CHECK(!player.SetCrossFadeEasing(E_OutBack));
	TEST_CHECK(player.Play("a", AnimLoopMode(255)) == InvalidScenePlayAnimRef);
	scene.Update(time_from_ms(100));
	CheckX(node, 20);
	const auto restarted = player.Play("c", ALM_Loop, true);
	TEST_CHECK(restarted != cr && player.IsTransitioning());
	scene.Update(time_from_ms(200));
	TEST_CHECK(!scene.IsPlaying(cr));
	// Expired handles must never stop later requests reusing the same storage slot.
	scene.StopAnim(cr);
	TEST_CHECK(scene.IsPlaying(restarted));
	player.Play("a");
	const auto latest = player.Play("b");
	scene.Update(0);
	CheckX(node, 20);
	scene.Update(time_from_ms(100));
	CheckX(node, 15);
	TEST_CHECK(!scene.IsPlaying(latest));
	scene.Update(time_from_ms(100));
	CheckX(node, 10);
}

static void TestSparsePoseAndRotation() {
	Scene scene;
	auto node = AnimNode(scene);
	node.GetTransform().SetPos({2, 0, 0});
	const auto a = PositionClip(scene, node, "a", 10, 10);
	Anim scale;
	scale.vec3_tracks.push_back({"Scale", {}});
	SetKey(scale.vec3_tracks.back(), time_ns(0), Vec3(3, 3, 3));
	SceneAnim clip;
	clip.name = "scale";
	clip.t_end = time_from_sec(1);
	clip.node_anims.push_back({node.ref, scene.AddAnim(scale)});
	const auto b = scene.AddSceneAnim(clip);
	auto player = scene.CreateAnimPlayer({a, b});
	player.SetCrossFadeDuration(time_from_ms(200));
	player.Play("a", ALM_Loop);
	scene.Update(0);
	player.Play("scale", ALM_Loop);
	scene.Update(time_from_ms(100));
	CheckX(node, 6);
	TEST_CHECK(std::abs(node.GetTransform().GetScale().x - 2) < 0.0001f);
	scene.Update(time_from_ms(100));
	CheckX(node, 2);
	scene.Update(time_from_ms(100));
	CheckX(node, 2);
	player.Stop();

	Scene rotations;
	auto bone = AnimNode(rotations);
	std::vector<SceneAnimRef> refs;
	for (float angle : {179.f, -179.f}) {
		Anim anim;
		anim.flags = AF_UseQuaternionForRotation;
		anim.quat_tracks.push_back({"Rotation", {}});
		SetKey(anim.quat_tracks.back(), time_ns(0), QuaternionFromEuler(0, Deg(angle), 0));
		SceneAnim r;
		r.name = angle > 0 ? "a" : "b";
		r.t_end = time_from_sec(1);
		r.node_anims.push_back({bone.ref, rotations.AddAnim(anim)});
		refs.push_back(rotations.AddSceneAnim(r));
	}
	auto rp = rotations.CreateAnimPlayer(refs);
	rp.SetCrossFadeDuration(time_from_ms(200));
	rp.Play("a", ALM_Loop);
	rotations.Update(0);
	rp.Play("b", ALM_Loop);
	rotations.Update(time_from_ms(100));
	const auto expected = QuaternionFromEuler(0, Pi, 0);
	TEST_CHECK(std::abs(Dot(expected, QuaternionFromEuler(bone.GetTransform().GetRot()))) > 0.9999f);
}

static void TestOwnershipLifecycleAndClocks() {
	Scene scene;
	auto n = AnimNode(scene);
	const auto a = PositionClip(scene, n, "a", 0, 0);
	const auto b = PositionClip(scene, n, "b", 10, 10);
	auto p = scene.CreateAnimPlayer({a, b});
	TEST_CHECK(!scene.CreateAnimPlayer({a}).IsValid());
	TEST_CHECK(scene.PlayAnim(a) == InvalidScenePlayAnimRef);
	p.SetCrossFadeDuration(time_from_ms(100));
	auto ar = p.Play("a", ALM_Loop);
	scene.Update(0);
	auto br = p.Play("b", ALM_Loop);
	scene.Update(time_from_ms(25));
	scene.StopAnim(ar); // Freeze the source; keep fading to B.
	TEST_CHECK(scene.IsPlaying(br));
	SceneClocks clocks;
	SceneUpdateSystems(scene, clocks, time_from_ms(25));
	CheckX(n, 5);
	scene.Update(time_from_sec(100));
	CheckX(n, 10);
	TEST_CHECK(!p.IsTransitioning());
	scene.StopAnim(br);
	TEST_CHECK(!scene.IsPlaying(br));
	auto legacy = scene.PlayAnim(a, ALM_Loop);
	TEST_CHECK(scene.IsPlaying(legacy));
	TEST_CHECK(p.Play("b") == InvalidScenePlayAnimRef);
	scene.StopAllAnims();
	TEST_CHECK(scene.GetPlayingAnimRefs().empty());
	TEST_CHECK(p.IsValid());
	p.Play("b");
	scene.Update(0);
	CheckX(n, 10);
	scene.DestroyAnim(scene.GetSceneAnim(a)->node_anims[0].anim);
	TEST_CHECK(!p.IsValid() && scene.GetPlayingAnimRefs().empty());
	auto p2 = scene.CreateAnimPlayer({b});
	TEST_CHECK(p2.IsValid());
	scene.DestroyNode(n.ref);
	TEST_CHECK(!p2.IsValid());
	scene.Clear();
	n = AnimNode(scene);
	const auto new_clip = PositionClip(scene, n, "new", 1, 1);
	auto p3 = scene.CreateAnimPlayer({new_clip});
	TEST_CHECK(p3.IsValid() && !p.IsValid() && !p2.IsValid());
	scene.Clear();
	TEST_CHECK(!p3.IsValid());

	Scene immediate;
	auto bone = AnimNode(immediate);
	const auto zero = PositionClip(immediate, bone, "zero", 1, 1, 0);
	auto z = immediate.CreateAnimPlayer({zero});
	TEST_CHECK(z.Play("zero", ALM_Loop) == InvalidScenePlayAnimRef);
	const auto once = z.Play("zero");
	immediate.Update(0);
	CheckX(bone, 1);
	TEST_CHECK(!immediate.IsPlaying(once));
}

static void TestImportAndRoundTrip() {
	Scene source, destination;
	auto s = AnimNode(source), d = AnimNode(destination);
	auto sc = AnimNode(source, "unanimated"), dc = AnimNode(destination, "unanimated");
	sc.GetTransform().SetParent(s.ref);
	dc.GetTransform().SetParent(d.ref);
	sc.GetTransform().SetPos({7, 0, 0});
	s.GetTransform().SetScale({2, 3, 4});
	s.GetTransform().SetRot({0, 0.3f, 0});
	const auto clip = PositionClip(source, s, "source", 0, 10);
	const auto mapping = BuildSceneAnimNodeMap(source, destination);
	TEST_ASSERT(mapping.success);
	const auto imported = ImportSceneAnim(source, clip, destination, mapping, "walk");
	TEST_ASSERT(imported.success);
	TEST_CHECK(imported.completed_channels == 5); // Includes the source defaults for an entirely unanimated node.
	const auto count = destination.GetAnims().size();
	TEST_CHECK(!ImportSceneAnim(source, clip, destination, mapping, "walk").success);
	TEST_CHECK(destination.GetAnims().size() == count);
	Scene foreign;
	AnimNode(foreign);
	auto bad_map = mapping;
	bad_map.destination_nodes[0] = foreign.GetNode("bone");
	TEST_CHECK(!ImportSceneAnim(source, clip, destination, bad_map, "bad").success);
	TEST_CHECK(destination.GetAnims().size() == count);
	SceneAnimNodeMap empty;
	empty.success = true;
	TEST_CHECK(!ImportSceneAnim(source, clip, destination, empty, "empty").success);
	source.Clear();
	auto player = destination.CreateAnimPlayer({imported.anim});
	TEST_ASSERT(player.IsValid());
	player.Play("walk");
	destination.Update(time_from_ms(500));
	CheckX(d, 5);
	CheckX(dc, 7);
	TEST_CHECK(std::abs(d.GetTransform().GetScale().z - 4) < 0.0001f);
	TEST_CHECK(std::abs(d.GetTransform().GetRot().y - 0.3f) < 0.0001f);
	player.Stop();
	PipelineResources resources;
	Data data;
	TEST_ASSERT(SaveSceneBinaryToData(data, destination, resources));
	data.Rewind();
	Scene loaded;
	LoadSceneContext context;
	TEST_ASSERT(LoadSceneBinaryFromData(data, "memory", loaded, g_file_reader, g_file_read_provider, resources, GetForwardPipelineInfo(), context));
	const auto info = GetSceneAnimInfo(loaded, loaded.GetSceneAnim("walk"));
	TEST_CHECK(info.valid && info.t_end == time_from_sec(1));
	auto lp = loaded.CreateAnimPlayer({loaded.GetSceneAnim("walk")});
	lp.Play("walk");
	loaded.Update(time_from_ms(500));
	CheckX(loaded.GetNode("bone"), 5);
	CheckX(loaded.GetNode("unanimated"), 7);
}

static void TestEasingAndTimeSteps() {
	for (const auto easing : {E_Linear, E_SmoothStep}) {
		Scene scene;
		auto node = AnimNode(scene);
		const auto a = PositionClip(scene, node, "a", 0, 0);
		const auto b = PositionClip(scene, node, "b", 8, 8);
		auto player = scene.CreateAnimPlayer({a, b});
		player.SetCrossFadeDuration(time_from_ms(400));
		TEST_CHECK(player.SetCrossFadeEasing(easing));
		player.Play("a", ALM_Infinite);
		scene.Update(time_from_sec(2));
		const auto br = player.Play("b", ALM_Infinite);
		scene.Update(time_from_ms(100));
		CheckX(node, easing == E_Linear ? 2.f : 1.25f);
		scene.Update(time_from_ms(1900));
		CheckX(node, 8);
		TEST_CHECK(scene.IsPlaying(br) && !player.IsTransitioning());
		player.SetCrossFadeDuration(0);
		player.Play("a");
		scene.Update(0);
		CheckX(node, 0);
	}
	// A very large loop step and many small steps must end on the same pose.
	for (bool subdivide : {false, true}) {
		Scene scene;
		auto node = AnimNode(scene);
		auto player = scene.CreateAnimPlayer({PositionClip(scene, node, "a", 0, 10)});
		player.Play("a", ALM_Loop);
		if (subdivide) {
			for (int i = 0; i < 2025; ++i)
				scene.Update(time_from_ms(50));
		} else
			scene.Update(time_from_ms(101250));
		CheckX(node, 2.03125f); // Hermite interpolation at t = 0.25, with zero tension and bias.
	}
	Scene scene;
	auto a = AnimNode(scene, "a"), b = AnimNode(scene, "b");
	b.SetTransform(a.GetTransform());
	TEST_CHECK(!scene.CreateAnimPlayer({PositionClip(scene, a, "a", 0, 1), PositionClip(scene, b, "b", 0, 1)}).IsValid());
}

void test_scene_animation() {
	TestLoopCombinations();
	TestEndpointsAndInterruptions();
	TestSparsePoseAndRotation();
	TestOwnershipLifecycleAndClocks();
	TestImportAndRoundTrip();
	TestEasingAndTimeSteps();
}

// Native runtime oracle for importer animation integration tests; no renderer.
#include <engine/scene.h>
#include <engine/node.h>
#include <engine/render_pipeline.h>
#include <engine/forward_pipeline.h>
#include <foundation/quaternion.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <json.hpp>

static void CheckLegacyAndExplicitInterpolation() {
	for (uint8_t flags : {uint8_t(0), hg::AF_TransformLinearInterpolation, hg::AF_TransformStepInterpolation}) {
		hg::Scene scene;
		auto node = scene.CreateNode("test");
		node.SetTransform(scene.CreateTransform());
		hg::Anim anim;
		anim.flags = flags | hg::AF_UseQuaternionForRotation;
		anim.vec3_tracks.push_back({"Position", {{0, {0, 0, 0}}, {hg::time_from_sec(1), {10, 0, 0}}}});
		anim.quat_tracks.push_back({"Rotation", {{0, {0, 0, 0, 1}}, {hg::time_from_sec(1), {0, 0, 1, 0}}}});
		hg::SceneAnim clip;
		clip.node_anims.push_back({node.ref, scene.AddAnim(anim)});
		scene.EvaluateBoundAnim(scene.BindAnim(scene.AddSceneAnim(clip)), hg::time_from_ms(250));
		const float x = flags == hg::AF_TransformStepInterpolation ? 0.f : flags == hg::AF_TransformLinearInterpolation ? 2.5f : 2.03125f;
		const float z = flags == hg::AF_TransformStepInterpolation ? 0.f : flags == hg::AF_TransformLinearInterpolation ? std::sin(hg::Pi / 8) : 1.f / std::sqrt(10.f);
		const auto q = hg::QuaternionFromEuler(node.GetTransform().GetRot());
		if (std::abs(node.GetTransform().GetPos().x - x) > 1e-5f || std::abs(std::abs(q.z) - z) > 1e-5f)
			throw std::runtime_error("Legacy or explicit native interpolation changed unexpectedly");
	}
}

int main(int argc, const char **argv) {
	try {
		CheckLegacyAndExplicitInterpolation();
		if (argc != 4 && argc != 5)
			throw std::runtime_error("Usage: sample_scene_animation scene.scn requests.json output.json [--binary-roundtrip]");
		nlohmann::json requests;
		std::ifstream input(argv[2]);
		input >> requests;
		nlohmann::json output = nlohmann::json::object();
		for (auto request = requests.begin(); request != requests.end(); ++request) {
			hg::Scene scene;
			hg::PipelineResources resources;
			hg::LoadSceneContext context;
			if (!hg::LoadSceneFromFile(argv[1], scene, resources, hg::GetForwardPipelineInfo(), context,
					hg::LSSF_Nodes | hg::LSSF_Anims | hg::LSSF_DoNotLoadResources))
				throw std::runtime_error("Cannot load native scene");
			if (argc == 5) {
				if (std::string(argv[4]) != "--binary-roundtrip")
					throw std::runtime_error("Unknown sampling option");
				const auto binary_path = std::string(argv[3]) + ".scn";
				if (!hg::SaveSceneBinaryToFile(binary_path.c_str(), scene, resources, hg::LSSF_Nodes | hg::LSSF_Anims))
					throw std::runtime_error("Cannot save binary scene");
				scene.Clear();
				hg::LoadSceneContext binary_context;
				if (!hg::LoadSceneBinaryFromFile(binary_path.c_str(), scene, resources, hg::GetForwardPipelineInfo(), binary_context,
						hg::LSSF_Nodes | hg::LSSF_Anims | hg::LSSF_DoNotLoadResources))
					throw std::runtime_error("Cannot load binary scene");
			}
			bool found = false;
			for (auto ref : scene.GetSceneAnims()) {
				if (scene.GetSceneAnim(ref)->name != request.key())
					continue;
				found = true;
				const auto bound = scene.BindAnim(ref);
				for (const auto &time : request.value()) {
					scene.EvaluateBoundAnim(bound, time.get<hg::time_ns>());
					nlohmann::json nodes = nlohmann::json::object();
					for (auto node : scene.GetNodes()) {
						const auto transform = node.GetTransform();
						const auto p = transform.GetPos(), s = transform.GetScale();
						const auto q = hg::QuaternionFromEuler(transform.GetRot());
						nodes[node.GetName()] = {{"position", {p.x, p.y, p.z}}, {"scale", {s.x, s.y, s.z}}, {"rotation", {q.x, q.y, q.z, q.w}}};
					}
					output[request.key()].push_back({{"time", time}, {"nodes", nodes}});
				}
			}
			if (!found)
				throw std::runtime_error("Requested clip missing: " + request.key());
		}
		std::ofstream file(argv[3]);
		file << output.dump();
		if (!file)
			throw std::runtime_error("Cannot write native samples");
		return 0;
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return 1;
	}
}

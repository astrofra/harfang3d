// Exercise production geometry and animation export without a renderer or asset files.
#define main gltf_exporter_cli_main
#include "../gltf_exporter.cpp"
#undef main

#include <stdexcept>
#include <json.hpp>

static void Check(bool condition, const char *message) {
	if (!condition)
		throw std::runtime_error(message);
}

static hg::Geometry Quad(bool triangles = false) {
	hg::Geometry geo;
	geo.vtx = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
	geo.pol = triangles ? std::vector<hg::Geometry::Polygon>{{3, 0}, {3, 0}} : std::vector<hg::Geometry::Polygon>{{4, 0}};
	geo.binding = triangles ? std::vector<uint32_t>{0, 1, 2, 0, 2, 3} : std::vector<uint32_t>{0, 1, 2, 3};
	geo.normal.assign(geo.binding.size(), {0, 0, 1});
	for (auto id : geo.binding)
		geo.uv[0].push_back({geo.vtx[id].x, geo.vtx[id].y});
	return geo;
}

static uint32_t Index(const Model &model, const Primitive &primitive, size_t i) {
	const auto &accessor = model.accessors[primitive.indices];
	const auto &view = model.bufferViews[accessor.bufferView];
	uint32_t value;
	std::memcpy(&value, model.buffers[view.buffer].data.data() + view.byteOffset + accessor.byteOffset + i * sizeof(value), sizeof(value));
	return value;
}

static void CheckExport(const hg::Geometry &geo, size_t vertices, size_t primitives = 1) {
	Model model;
	Mesh mesh;
	Check(ExportGeometry(model, mesh, geo), "export failed");
	Check(mesh.primitives.size() == primitives, "wrong primitive count");
	for (const auto &primitive : mesh.primitives) {
		Check(model.accessors[primitive.attributes.at("POSITION")].count == vertices, "wrong vertex count");
		for (const auto &attribute : primitive.attributes)
			Check(model.accessors[attribute.second].count == vertices, "attribute count mismatch");
		const auto &indices = model.accessors[primitive.indices];
		Check(indices.count > 0, "empty primitive");
		for (size_t i = 0; i < indices.count; ++i)
			Check(Index(model, primitive, i) < vertices, "index out of range");
	}
}

static std::vector<float> AnimationValues(const Model &model, int index) {
	const auto &a = model.accessors[index];
	const auto &view = model.bufferViews[a.bufferView];
	const size_t components = a.type == TINYGLTF_TYPE_MAT4 ? 16 : a.type == TINYGLTF_TYPE_VEC4 ? 4 : a.type == TINYGLTF_TYPE_VEC3 ? 3 : 1;
	std::vector<float> values(a.count * components);
	std::memcpy(values.data(), model.buffers[view.buffer].data.data() + view.byteOffset + a.byteOffset, values.size() * sizeof(float));
	return values;
}

static void TestAnimationTracks() {
	hg::Scene scene;
	const auto node = scene.CreateNode("animated");
	const auto hidden = scene.CreateNode("unexported");
	NodeRef_to_IdNode.clear();
	NodeRef_to_IdNode[node.ref] = 2;
	hg::Anim anim;
	anim.flags = hg::AF_UseQuaternionForRotation;
	// Deliberately shuffled, including an unsupported target and inactive Euler track.
	anim.vec3_tracks = {{"Unknown", {}}, {"Scale", {}}, {"Rotation", {}}, {"Position", {}}};
	hg::SetKey(anim.vec3_tracks[1], hg::time_from_sec(0), hg::Vec3(1, 2, 3));
	hg::SetKey(anim.vec3_tracks[1], hg::time_from_sec(2), hg::Vec3(-1, 4, -2));
	hg::SetKey(anim.vec3_tracks[2], hg::time_from_sec(0), hg::Vec3(1, 2, 3));
	hg::SetKey(anim.vec3_tracks[3], hg::time_from_sec(0), hg::Vec3(1, 2, 3));
	hg::SetKey(anim.vec3_tracks[3], hg::time_from_sec(2), hg::Vec3(4, 5, 6));
	anim.quat_tracks = {{"Rotation", {}}};
	hg::SetKey(anim.quat_tracks[0], hg::time_from_sec(0), hg::Quaternion(0, 0, 0, 1));
	hg::SetKey(anim.quat_tracks[0], hg::time_from_sec(2), hg::QuaternionFromEuler(0, 0, hg::Pi / 2));
	hg::SceneAnim clip;
	clip.name = "TRS";
	const auto ref = scene.AddAnim(anim);
	clip.node_anims = {{node.ref, ref}, {hidden.ref, ref}};
	scene.AddSceneAnim(clip);
	anim.flags = 0; // Native Euler rotation mode despite a quaternion track being present.
	anim.vec3_tracks[2].keys.clear();
	hg::SetKey(anim.vec3_tracks[2], hg::time_from_sec(0), hg::Vec3(hg::Pi / 2, 0, 0));
	clip.name = "Euler";
	clip.node_anims = {{node.ref, scene.AddAnim(anim)}};
	scene.AddSceneAnim(clip);
	Model model;
	ExportMotions(model, scene);
	Check(model.animations.size() == 2, "clips lost");
	Check(model.animations[0].name == "TRS" && model.animations[1].name == "Euler", "clip names changed");
	Check(NodeRef_to_IdNode.count(hidden.ref) == 0, "unexported target inserted as node zero");
	for (const auto &animation : model.animations) {
		Check(animation.channels.size() == 3, "missing or duplicate TRS channel");
		std::set<std::string> paths;
		for (const auto &channel : animation.channels) {
			Check(channel.target_node == 2, "wrong animation target");
			Check(paths.insert(channel.target_path).second, "duplicate target path");
			const auto &sampler = animation.samplers[channel.sampler];
			const auto values = AnimationValues(model, sampler.output);
			if (channel.target_path == "translation")
				Check(values == std::vector<float>({1, 2, -3, 4, 5, -6}), "translation samples lost or incorrectly reflected");
			else if (channel.target_path == "scale")
				Check(values == std::vector<float>({1, 2, 3, -1, 4, -2}), "scale samples reflected");
			else {
				Check(model.accessors[sampler.output].type == TINYGLTF_TYPE_VEC4, "rotation is not quaternion data");
				if (animation.name == "TRS")
					Check(model.accessors[sampler.output].maxValues[3] == 1, "quaternion W bound incorrect");
				if (animation.name == "Euler")
					Check(values.size() == 4 && std::abs(values[0] + std::sqrt(0.5f)) < 1e-6f &&
						std::abs(values[1]) < 1e-6f && std::abs(values[2]) < 1e-6f && std::abs(values[3] - std::sqrt(0.5f)) < 1e-6f,
						"native Euler rotation mode or coordinate conversion incorrect");
			}
		}
	}

	// No partial accessors/channels for empty tracks or float-time collisions.
	Model invalid_model;
	Animation invalid_animation;
	hg::AnimTrackHermiteT<hg::Vec3> track{"Position", {}};
	const auto convert = [](const hg::Vec3 &v) { return std::array<float, 3>{v.x, v.y, v.z}; };
	ExportAnimationTrack(invalid_model, invalid_animation, 0, "translation", &track, convert);
	hg::SetKey(track, hg::time_from_sec(1), hg::Vec3(0, 0, 0));
	hg::SetKey(track, hg::time_from_sec(1) + 1, hg::Vec3(1, 0, 0));
	ExportAnimationTrack(invalid_model, invalid_animation, 0, "translation", &track, convert);
	Check(invalid_model.accessors.empty() && invalid_animation.channels.empty(), "invalid track committed partial data");
	NodeRef_to_IdNode.clear();
}

static void TestSkinExport() {
	auto seam = Quad(true);
	seam.normal[3] = {1, 0, 0};
	Model seam_model;
	Mesh seam_mesh;
	std::vector<uint32_t> source_vertices;
	Check(ExportGeometry(seam_model, seam_mesh, seam, &source_vertices), "seam geometry export failed");
	Check(source_vertices == std::vector<uint32_t>({0, 2, 1, 0, 3}), "native identity lost at a seam");
	seam.skin.resize(3);
	Check(!ExportGeometry(seam_model, seam_mesh, seam), "non-vertex-indexed skin records accepted");
	hg::Scene scene;
	const auto a = scene.CreateNode("joint_a"), b = scene.CreateNode("joint_b"), other = scene.CreateNode("instance_joint");
	NodeRef_to_IdNode.clear();
	NodeRef_to_IdNode[a.ref] = 3;
	NodeRef_to_IdNode[b.ref] = 4;
	NodeRef_to_IdNode[other.ref] = 5;
	ExportGeometryCache geo;
	geo.source_vertices = {1, 0, 1}; // a seam split must receive the same source weights
	geo.bind_pose = {hg::TranslationMat4({0, 0, 2}), hg::Mat4::Identity, hg::TranslationMat4({0, 0, 2})};
	geo.skin.resize(2);
	geo.skin[0] = {{0, 2, 1, 0}, {100, 100, 50, 0}};
	geo.skin[1] = {{1, 0, 0, 0}, {255, 0, 0, 0}};
	std::vector<hg::NodeRef> bones = {a.ref, b.ref, a.ref};
	Model model;
	Mesh mesh;
	mesh.primitives.emplace_back();
	int skin_id = -1;
	Check(ExportObjectSkin(model, mesh, geo, bones, skin_id), "skin export failed");
	Check(model.skins[skin_id].joints == std::vector<int>({3, 4}), "duplicate joint not canonicalized");
	const auto weights = AnimationValues(model, mesh.primitives[0].attributes.at("WEIGHTS_0"));
	Check(weights == std::vector<float>({1, 0, 0, 0, .8f, .2f, 0, 0, 1, 0, 0, 0}), "skin remap/normalization/seam propagation failed");
	const auto &ja = model.accessors[mesh.primitives[0].attributes.at("JOINTS_0")];
	Check(ja.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, "joint indices must support 16-bit palettes");
	const auto &jv = model.bufferViews[ja.bufferView];
	uint16_t joints[12];
	std::memcpy(joints, model.buffers[jv.buffer].data.data(), sizeof(joints));
	Check(joints[0] == 1 && joints[4] == 0 && joints[5] == 1 && joints[8] == 1, "joint slot remap failed");
	const auto &ma = model.accessors[model.skins[skin_id].inverseBindMatrices];
	const auto &mv = model.bufferViews[ma.bufferView];
	float matrices[32];
	std::memcpy(matrices, model.buffers[mv.buffer].data.data(), sizeof(matrices));
	Check(ma.type == TINYGLTF_TYPE_MAT4 && ma.count == 2 && matrices[14] == -2.f && matrices[15] == 1.f && matrices[31] == 1.f,
		"bind matrix layout/reflection incorrect");
	// Object instances may share geometry but must resolve their own node palette.
	bones = {other.ref, b.ref, other.ref};
	Mesh instance = mesh;
	Check(ExportObjectSkin(model, instance, geo, bones, skin_id), "second object skin failed");
	Check(model.skins[skin_id].joints == std::vector<int>({5, 4}) && model.skins[0].joints[0] == 3, "object palettes were shared incorrectly");
	const auto buffers_before = model.buffers.size();
	geo.bind_pose[2] = hg::Mat4::Identity;
	Check(!ExportObjectSkin(model, instance, geo, bones, skin_id), "conflicting bind poses accepted");
	geo.bind_pose[2] = geo.bind_pose[0];
	geo.skin[1].weight[0] = 0;
	Check(!ExportObjectSkin(model, instance, geo, bones, skin_id), "zero weights accepted");
	geo.skin[1].weight[0] = 255;
	geo.skin[1].index[0] = 99;
	Check(!ExportObjectSkin(model, instance, geo, bones, skin_id), "invalid joint slot accepted");
	geo.skin[1].index[0] = 1;
	NodeRef_to_IdNode.erase(b.ref);
	Check(!ExportObjectSkin(model, instance, geo, bones, skin_id), "missing joint accepted");
	Check(model.buffers.size() == buffers_before, "failed skin committed partial buffers");
	NodeRef_to_IdNode.clear();
}

// Native evaluator oracle for CPU deformation comparisons (no renderer needed).
static void WriteSkinReference(const char *scene_path, const char *geo_path, const char *output_path) {
	hg::Scene scene;
	hg::PipelineResources resources;
	hg::LoadSceneContext context;
	Check(hg::LoadSceneJsonFromFile(scene_path, scene, resources, hg::GetForwardPipelineInfo(), context,
		hg::LSSF_Nodes | hg::LSSF_Anims | hg::LSSF_DoNotLoadResources), "cannot load reference scene");
	const auto geo = hg::LoadGeometryFromFile(geo_path);
	Check(!geo.skin.empty() && geo.skin.size() == geo.vtx.size(), "reference requires skinned geometry");
	const auto object = scene.GetNode("Layer").GetObject();
	Check(object.GetBoneCount() == geo.bind_pose.size(), "reference bone count mismatch");
	nlohmann::json output = nlohmann::json::array();
	for (const auto ref : scene.GetSceneAnims()) {
		const auto *clip = scene.GetSceneAnim(ref);
		const auto bound = scene.BindAnim(ref);
		nlohmann::json record;
		record["clip"] = clip->name;
		record["samples"] = nlohmann::json::array();
		for (int sample = 0; sample <= 8; ++sample) {
			const auto time = clip->t_start + (clip->t_end - clip->t_start) * sample / 8;
			scene.EvaluateBoundAnim(bound, time);
			std::vector<hg::Mat4> matrices;
			nlohmann::json joint_worlds = nlohmann::json::object();
			for (size_t slot = 0; slot < object.GetBoneCount(); ++slot) {
				const auto world = scene.ComputeNodeWorldMatrix(object.GetBone(slot));
				matrices.push_back(world * geo.bind_pose[slot]);
				std::vector<float> values;
				for (const auto &row : world.m)
					values.insert(values.end(), std::begin(row), std::end(row));
				joint_worlds[object.GetBoneNode(slot).GetName()] = values;
			}
			nlohmann::json positions = nlohmann::json::array();
			for (size_t vertex = 0; vertex < geo.vtx.size(); ++vertex) {
				hg::Vec3 position = hg::Vec3::Zero;
				unsigned total = 0;
				for (int i = 0; i < 4; ++i) {
					position += (matrices[geo.skin[vertex].index[i]] * geo.vtx[vertex]) * float(geo.skin[vertex].weight[i]);
					total += geo.skin[vertex].weight[i];
				}
				position /= float(total);
				positions.push_back({position.x, position.y, -position.z});
			}
			record["samples"].push_back({{"time", hg::time_to_sec_f(time)}, {"positions", positions}, {"joint_worlds", joint_worlds}});
		}
		output.push_back(record);
	}
	std::ofstream file(output_path);
	file << output.dump();
	Check(bool(file), "cannot write reference samples");
}

int main(int argc, const char **argv) {
	try {
		if (argc == 5 && std::string(argv[1]) == "--skin-reference") {
			WriteSkinReference(argv[2], argv[3], argv[4]);
			return 0;
		}
		TestSkinExport();
		TestAnimationTracks();
		auto geo = Quad();
		CheckExport(geo, 4);
		Model model;
		Mesh mesh;
		Check(ExportGeometry(model, mesh, geo), "quad export failed");
		// First occurrence ordering is 0,2,1,3; shared diagonal must reuse 0 and 1.
		const uint32_t expected[] = {0, 1, 2, 0, 3, 1};
		for (size_t i = 0; i < 6; ++i)
			Check(Index(model, mesh.primitives[0], i) == expected[i], "quad winding/reuse changed");
		geo = Quad(true);
		CheckExport(geo, 4);
		geo.uv[0][3].x = 0.5f;
		CheckExport(geo, 5); // UV seam
		geo = Quad(true);
		geo.normal[3] = {1, 0, 0};
		CheckExport(geo, 5); // hard normal
		geo = Quad(true);
		geo.uv[1].assign(6, {0, 0});
		geo.uv[1][3].x = 1;
		CheckExport(geo, 5); // secondary UV seam
		geo = Quad(true);
		geo.uv[2].assign(6, {0, 0});
		geo.uv[2][3].x = 1;
		CheckExport(geo, 4); // unexported attributes must not force splits
		geo = Quad(true);
		geo.vtx.push_back(geo.vtx[0]);
		geo.binding[3] = 4;
		CheckExport(geo, 5); // coincident but disconnected native IDs
		geo = Quad(true);
		geo.uv[0][3].x = -0.f;
		CheckExport(geo, 4); // signed-zero hashing agrees with equality
		geo.pol[1].material = 255;
		CheckExport(geo, 4, 2); // sparse slots, full uint8 range, cross-material reuse
		model = Model{};
		mesh = Mesh{};
		Check(ExportGeometry(model, mesh, geo), "material export failed");
		Check(mesh.primitives[1].material == 255, "native material slot lost");
		geo = Quad();
		geo.normal.clear();
		model = Model{};
		mesh = Mesh{};
		Check(ExportGeometry(model, mesh, geo), "missing normals export failed");
		Check(mesh.primitives[0].attributes.count("NORMAL") == 0, "fabricated normals");
		geo = Quad();
		geo.binding[0] = 99;
		Check(!ExportGeometry(model, mesh, geo), "invalid binding accepted");
		geo = Quad();
		geo.normal.pop_back();
		Check(!ExportGeometry(model, mesh, geo), "invalid attribute count accepted");
		geo = Quad();
		geo.uv[0][0].x = std::numeric_limits<float>::infinity();
		Check(!ExportGeometry(model, mesh, geo), "non-finite UV accepted");
		geo = Quad();
		geo.pol = {{2, 0}};
		geo.binding.resize(2);
		geo.normal.resize(2);
		geo.uv[0].resize(2);
		Check(!ExportGeometry(model, mesh, geo), "two-corner polygon accepted");
		geo = {};
		Check(!ExportGeometry(model, mesh, geo), "empty geometry accepted");
		std::cout << "glTF exporter geometry, animation and skin tests passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}

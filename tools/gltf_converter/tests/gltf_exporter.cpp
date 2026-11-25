// Exercise the production geometry export path without a renderer or asset files.
#define main gltf_exporter_cli_main
#include "../gltf_exporter.cpp"
#undef main

#include <stdexcept>

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

int main() {
	try {
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
		std::cout << "glTF exporter geometry tests passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}

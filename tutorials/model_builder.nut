// Model builder usage

local hg = require("harfang");

function create_grid_model_with_model_builder(vtx_layout, origin_pos, quad_size, range_x, range_z, center_on_origin, time) {
	local vertex_positions = compute_grid_vertex_position(origin_pos, quad_size, range_x, range_z, center_on_origin, time);
	local grid_triangles = compute_triangles_for_grid_vertex(range_x, range_z);
	local vertex_normals = compute_vertex_normals(grid_triangles, vertex_positions);

	local mdl_builder = hg.ModelBuilder();
	local vertex_indices = [];

	for (local i = 0; i < vertex_positions.len(); ++i) {
		local v = hg.Vertex();
		v.pos = vertex_positions[i];
		v.normal = vertex_normals[i];
		vertex_indices.append(mdl_builder.AddVertex(v));
	}

	for (local i = 0; i < grid_triangles.len(); ++i) {
		local triangle = grid_triangles[i];
		mdl_builder.AddTriangle(vertex_indices[triangle[0]], vertex_indices[triangle[1]], vertex_indices[triangle[2]]);
	}

	mdl_builder.EndList(0);
	return mdl_builder.MakeModel(vtx_layout);
}

function compute_grid_vertex_position(origin_pos, quad_size, range_x, range_z, center_on_origin, time) {
	local offset = center_on_origin ? hg.Vec3(range_x * quad_size / 2.0, 0, range_z * quad_size / 2.0) : hg.Vec3(0, 0, 0);
	local positions = [];

	for (local iz = 0; iz <= range_z; ++iz) {
		for (local ix = 0; ix <= range_x; ++ix) {
			local pos_x = origin_pos.x + ix * quad_size - offset.x;
			local pos_z = origin_pos.z + iz * quad_size - offset.z;
			local pos_y = sin(pos_x) * sin(pos_z) * sin(time);
			positions.append(hg.Vec3(pos_x, pos_y, pos_z));
		}
	}

	return positions;
}

function compute_triangles_for_grid_vertex(range_x, range_z) {
	local triangles = [];

	function get_vertex_id(ix, iz) {
		return iz * (range_x + 1) + ix;
	}

	for (local iz = 0; iz < range_z; ++iz) {
		for (local ix = 0; ix < range_x; ++ix) {
			local a = get_vertex_id(ix, iz);
			local b = get_vertex_id(ix, iz + 1);
			local c = get_vertex_id(ix + 1, iz + 1);
			local d = get_vertex_id(ix + 1, iz);

			triangles.append([d, c, b]);
			triangles.append([b, a, d]);
		}
	}

	return triangles;
}

function compute_vertex_normals(triangles, positions) {
	local normals = [];
	for (local i = 0; i < positions.len(); ++i) {
		normals.append(hg.Vec3(0, 0, 0));
	}

	for (local it = 0; it < triangles.len(); ++it) {
		local triangle = triangles[it];
		local v0 = triangle[0];
		local v1 = triangle[1];
		local v2 = triangle[2];

		local p0 = positions[v0];
		local p1 = positions[v1];
		local p2 = positions[v2];
		local face_normal = hg.Cross(p0 - p1, p2 - p1);

		if (face_normal.x == 0 && face_normal.y == 0 && face_normal.z == 0) {
			continue;
		}

		normals[v0] = normals[v0] + face_normal;
		normals[v1] = normals[v1] + face_normal;
		normals[v2] = normals[v2] + face_normal;
	}

	for (local i = 0; i < normals.len(); ++i) {
		if (hg.Len(normals[i]) > 0) {
			normals[i] = hg.Normalize(normals[i]);
		} else {
			normals[i] = hg.Vec3(0, 1, 0);
		}
	}

	return normals;
}

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Model builder", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local prg_ref = hg.LoadPipelineProgramRefFromAssets("core/shader/pbr.hps", res, hg.GetForwardPipelineInfo());
local plane_material = hg.CreateMaterial(prg_ref, "uBaseOpacityColor", hg.Vec4(0.5, 0.5, 0.5), "uOcclusionRoughnessMetalnessColor", hg.Vec4(1, 1, 0.25));

local scene = hg.Scene();
hg.LoadSceneFromAssets("probe_scene/pbr.scn", scene, res, hg.GetForwardPipelineInfo());

local cam = hg.CreateCamera(scene, hg.TransformationMat4(hg.Vec3(0, 6, -12), hg.Vec3(hg.DegreeToRadian(30), 0, 0)), 0.01, 1000);
scene.SetCurrentCamera(cam);

local light_mtx = hg.TransformationMat4(hg.Vec3(8, 5, 0), hg.Vec3(hg.DegreeToRadian(50), hg.DegreeToRadian(-90), hg.DegreeToRadian(-90)));
local inner_angle = hg.DegreeToRadian(30);
local outer_angle = hg.DegreeToRadian(45);
local light_color = hg.Color(1, 1, 1, 1);
local light = hg.CreateSpotLight(scene, light_mtx, 0, inner_angle, outer_angle, light_color, 1, light_color, 1, 1, hg.LST_Map, 0.0);

local sphere_node = scene.GetNode("sphere");

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();
local grid_start_pos = hg.Vec3(0, 0, 0);
local quad_size = 0.25;
local grid_mdl = create_grid_model_with_model_builder(vtx_layout, grid_start_pos, quad_size, 40, 40, true, 1);
local grid_mdl_ref = res.AddModel("grid", grid_mdl);
local grid_node = hg.CreateObject(scene, hg.TransformationMat4(grid_start_pos, hg.Vec3(0, 0, 0)), grid_mdl_ref, [plane_material]);

local keyboard = hg.Keyboard();

while (!keyboard.Down(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();
	local dts = hg.time_to_sec_f(dt);
	local current_time = hg.time_to_sec_f(hg.GetClock());

	local new_grid_mdl = create_grid_model_with_model_builder(vtx_layout, grid_start_pos, quad_size, 40, 40, true, current_time);
	res.UpdateModel(grid_mdl_ref, new_grid_mdl);

	scene.Update(dt);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
	local view_id = submit_result[0];

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);

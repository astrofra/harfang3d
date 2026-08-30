// Model builder with iso surface

local hg = require("harfang");

local PI = 3.141592653589793;
local anim_spheres = true;

function create_iso_surface_sphere_default() {
	return {
		iso_sphere_pos = hg.Vec3(0, 0, 0),
		iso_sphere_radius = 1.0,
		iso_sphere_value = 1.0,
		iso_sphere_exponent = 1.0
	};
}

function create_iso_surface_sphere(pos, radius) {
	return {
		iso_sphere_pos = pos,
		iso_sphere_radius = radius,
		iso_sphere_value = 1.0,
		iso_sphere_exponent = 1.0
	};
}

function create_iso_surface_spheres_list_circle(origin) {
	local sphere_list = [];
	local sphere_radius = 8.0;
	local radius = 15.0;

	for (local i = 0; i < 10; ++i) {
		local angle = i / 10.0 * PI * 2.0;
		local x = radius * cos(angle);
		local y = radius * sin(angle);
		local z = i * radius / 6.0;
		sphere_list.append(create_iso_surface_sphere(hg.Vec3(x, y, z) + origin, sphere_radius));
	}

	return sphere_list;
}

function create_iso_surface_with_spheres_list(iso_surface_bounds, iso_level, iso_scale, iso_spheres_list, clock_sec) {
	local iso_surface_width = iso_surface_bounds[0];
	local iso_surface_height = iso_surface_bounds[1];
	local iso_surface_depth = iso_surface_bounds[2];
	local iso_surface = hg.NewIsoSurface(iso_surface_width, iso_surface_height, iso_surface_depth);

	for (local i = 0; i < iso_spheres_list.len(); ++i) {
		local iso_sphere = iso_spheres_list[i];
		local iso_sphere_pos = iso_sphere.iso_sphere_pos;
		local iso_sphere_radius = iso_sphere.iso_sphere_radius;
		local iso_sphere_value = iso_sphere.iso_sphere_value;
		local iso_sphere_exponent = iso_sphere.iso_sphere_exponent;

		local pos_z = iso_sphere_pos.z;
		if (anim_spheres) {
			local clock = cos(clock_sec + i) / 4.0;
			pos_z = ((iso_surface_height / 2.0 - iso_sphere_radius) * clock) + iso_surface_height / 4.0;
		}

		hg.IsoSurfaceSphere(
			iso_surface,
			iso_surface_width, iso_surface_depth, iso_surface_height,
			iso_sphere_pos.x, iso_sphere_pos.y, pos_z,
			iso_sphere_radius, iso_sphere_value, iso_sphere_exponent
		);
	}

	local mdl_builder = hg.ModelBuilder();
	local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();
	local material_idx = 0;
	local iso_surface_result = hg.IsoSurfaceToModel(
		mdl_builder,
		iso_surface,
		iso_surface_width, iso_surface_height, iso_surface_depth,
		material_idx,
		iso_level,
		iso_scale.x, iso_scale.y, iso_scale.z
	);

	return mdl_builder.MakeModel(vtx_layout);
}

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Model builder iso surface", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

local pipeline = hg.CreateForwardPipeline(1024, true);
local res = hg.PipelineResources();

hg.AddAssetsFolder("resources_compiled");

local imgui_prg = hg.LoadProgramFromAssets("core/shader/imgui");
local imgui_img_prg = hg.LoadProgramFromAssets("core/shader/imgui_image");
hg.ImGuiInit(10, imgui_prg, imgui_img_prg);

local scene = hg.Scene();
hg.LoadSceneFromAssets("probe_scene/scene_iso_surface.scn", scene, res, hg.GetForwardPipelineInfo());

local camera_rot_x = 0.0;
local camera_rot_y = 0.0;
local camera_distance = 45;
local camera_new_mtx = hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(camera_rot_x, camera_rot_y, 0)) *
	hg.TransformationMat4(hg.Vec3(0, 0, -camera_distance), hg.Vec3(0, 0, 0));
local camera_node = hg.CreateCamera(scene, camera_new_mtx, 0.5, 800);
scene.SetCurrentCamera(camera_node);

local ground_node = scene.GetNode("ground");

local spot_light_mtx = hg.TransformationMat4(hg.Vec3(12.5, 35, 12.5), hg.Vec3(hg.DegreeToRadian(90), hg.DegreeToRadian(90), hg.DegreeToRadian(0)));
local inner_angle = hg.DegreeToRadian(0.1);
local outer_angle = hg.DegreeToRadian(35);
local spot_light_color = hg.Color(1, 1, 1, 1);
local spot_diffuse_intensity = 10.0;
local spot_specular_intensity = 10.0;
local spot_light = hg.CreateSpotLight(scene, spot_light_mtx, 0,
	inner_angle, outer_angle,
	spot_light_color, spot_diffuse_intensity,
	spot_light_color, spot_specular_intensity,
	1, hg.LST_Map, 0.0001
);

local keyboard = hg.Keyboard();

local prg_ref = hg.LoadPipelineProgramRefFromAssets("core/shader/pbr.hps", res, hg.GetForwardPipelineInfo());
local iso_surface_material = hg.CreateMaterial(prg_ref, "uBaseOpacityColor", hg.Vec4(1.0, 0.75, 0.15), "uOcclusionRoughnessMetalnessColor", hg.Vec4(1, 0.2, 0.5));

local iso_surface_bounds = [50, 50, 50];
local iso_sphere_list = create_iso_surface_spheres_list_circle(hg.Vec3(iso_surface_bounds[0] / 2.0, iso_surface_bounds[0] / 2.0, iso_surface_bounds[0] / 6.0));

local iso_level = 0.8;
local iso_scale = hg.Vec3(1, 1, 1);

local iso_surface_mdl = create_iso_surface_with_spheres_list(iso_surface_bounds, iso_level, iso_scale, iso_sphere_list, 1);
local iso_surface_mdl_ref = res.AddModel("isosurface", iso_surface_mdl);

local iso_surface_node_scale = hg.Vec3(0.5, 0.5, 0.5);
local iso_surface_node = hg.CreateObject(scene, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(0, 0, 0), iso_surface_node_scale), iso_surface_mdl_ref, [iso_surface_material]);

local pipeline_aaa_config = hg.ForwardPipelineAAAConfig();
pipeline_aaa_config.exposure = 1.3;
pipeline_aaa_config.gamma = 0.8;
pipeline_aaa_config.bloom_threshold = 0.05;
pipeline_aaa_config.bloom_intensity = 0.7;
pipeline_aaa_config.sample_count = 1;
local pipeline_aaa = hg.CreateForwardPipelineAAAFromAssets("core", pipeline_aaa_config, hg.BR_Equal, hg.BR_Equal);

local frame = 0;

while (!keyboard.Down(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();
	local current_time = hg.time_to_sec_f(hg.GetClock());

	local iso_surface_width = iso_surface_bounds[0];
	local iso_surface_height = iso_surface_bounds[1];
	local iso_surface_depth = iso_surface_bounds[2];

	local scale_factor = iso_scale * iso_surface_node_scale;

	if (hg.ReadKeyboard().Key(hg.K_W)) {
		camera_rot_x += 1.0 * PI / 180.0;
	} else if (hg.ReadKeyboard().Key(hg.K_S)) {
		camera_rot_x -= 1.0 * PI / 180.0;
	} else if (hg.ReadKeyboard().Key(hg.K_A)) {
		camera_rot_y += 1.0 * PI / 180.0;
	} else if (hg.ReadKeyboard().Key(hg.K_D)) {
		camera_rot_y -= 1.0 * PI / 180.0;
	}

	camera_new_mtx = hg.TransformationMat4(
		hg.Vec3(iso_surface_width / 2.0 * scale_factor.x, iso_surface_height / 3.0 * scale_factor.y, iso_surface_depth / 2.0 * scale_factor.z),
		hg.Vec3(camera_rot_x, camera_rot_y, 0)
	) * hg.TransformationMat4(hg.Vec3(0, 0, -camera_distance), hg.Vec3(0, 0, 0));
	camera_node.GetTransform().SetWorld(camera_new_mtx);

	local ground_node_preview_mtx = hg.TransformationMat4(
		hg.Vec3(iso_surface_width / 2.0 * scale_factor.x, 0, iso_surface_depth / 2.0 * scale_factor.z),
		hg.Vec3(0, 0, 0),
		hg.Vec3(iso_surface_width * scale_factor.x * 2.0, 1, iso_surface_depth * scale_factor.z * 2.0)
	);
	ground_node.GetTransform().SetWorld(ground_node_preview_mtx);
	local ground_node_scale = ground_node.GetTransform().GetScale();

	local new_iso_surface_mdl = create_iso_surface_with_spheres_list(iso_surface_bounds, iso_level, iso_scale, iso_sphere_list, current_time);
	res.UpdateModel(iso_surface_mdl_ref, new_iso_surface_mdl);
	iso_surface_node.GetTransform().SetScale(iso_surface_node_scale);
	iso_surface_node_scale = iso_surface_node.GetTransform().GetScale();

	scene.Update(dt);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res, pipeline_aaa, pipeline_aaa_config, frame);
	local vid = submit_result[0];
	local passid = submit_result[1];

	hg.ImGuiBeginFrame(res_x, res_y, hg.TickClock(), hg.ReadMouse(), hg.ReadKeyboard());

	local begin_result = hg.ImGuiBegin("IsoSurface setting", true, hg.ImGuiWindowFlags_AlwaysAutoResize);
	if (begin_result[0]) {
		local input_camera_distance = hg.ImGuiInputInt("Camera distance", camera_distance);
		local changed = input_camera_distance[0];
		camera_distance = input_camera_distance[1];

		local input_model_scale = hg.ImGuiInputVec3("Model node scale", iso_surface_node_scale);
		changed = input_model_scale[0];
		iso_surface_node_scale = input_model_scale[1];

		hg.ImGuiText("ground size : x = " + ground_node_scale.x.tostring() + ", y = " + ground_node_scale.y.tostring() + ", z = " + ground_node_scale.z.tostring());
		hg.ImGuiNewLine();

		local anim_result = hg.ImGuiCheckbox("Anim spheres", anim_spheres);
		changed = anim_result[0];
		anim_spheres = anim_result[1];

		hg.ImGuiNewLine();

		if (hg.ImGuiCollapsingHeader("Iso surface")) {
			hg.ImGuiText("iso_surface_bounds");

			local width_result = hg.ImGuiInputInt("width", iso_surface_bounds[0]);
			changed = width_result[0];
			iso_surface_bounds[0] = width_result[1];

			local height_result = hg.ImGuiInputInt("height", iso_surface_bounds[1]);
			changed = height_result[0];
			iso_surface_bounds[1] = height_result[1];

			local depth_result = hg.ImGuiInputInt("depth", iso_surface_bounds[2]);
			changed = depth_result[0];
			iso_surface_bounds[2] = depth_result[1];

			local iso_level_result = hg.ImGuiInputFloat("iso_level", iso_level);
			changed = iso_level_result[0];
			iso_level = iso_level_result[1];

			local iso_scale_result = hg.ImGuiInputVec3("iso_scale", iso_scale);
			changed = iso_scale_result[0];
			iso_scale = iso_scale_result[1];
		}

		hg.ImGuiNewLine();

		for (local i = 0; i < iso_sphere_list.len(); ++i) {
			local iso_sphere = iso_sphere_list[i];
			local str_i = i.tostring();

			if (hg.ImGuiCollapsingHeader("Iso sphere" + str_i)) {
				local pos_result = hg.ImGuiInputVec3("iso_sphere_pos_" + str_i, iso_sphere.iso_sphere_pos);
				changed = pos_result[0];
				iso_sphere.iso_sphere_pos = pos_result[1];

				local radius_result = hg.ImGuiInputFloat("iso_sphere_radius_" + str_i, iso_sphere.iso_sphere_radius);
				changed = radius_result[0];
				iso_sphere.iso_sphere_radius = radius_result[1];

				local value_result = hg.ImGuiInputFloat("iso_sphere_value_" + str_i, iso_sphere.iso_sphere_value);
				changed = value_result[0];
				iso_sphere.iso_sphere_value = value_result[1];

				local exponent_result = hg.ImGuiInputFloat("iso_sphere_exponent_" + str_i, iso_sphere.iso_sphere_exponent);
				changed = exponent_result[0];
				iso_sphere.iso_sphere_exponent = exponent_result[1];
			}
		}
	}

	hg.ImGuiEnd();
	hg.ImGuiEndFrame(vid);

	frame = hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);

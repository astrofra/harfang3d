// Dynamically assign lights to the fixed pipeline slots by adjusting their priority

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Light priority relative to a specific world position", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

local light_mdl = hg.CreateSphereModel(vtx_layout, 0.05, 8, 16);
local light_ref = res.AddModel("light", light_mdl);
local orb_mdl = hg.CreateSphereModel(vtx_layout, 1, 16, 32);
local orb_ref = res.AddModel("orb", orb_mdl);
local ground_mdl = hg.CreateCubeModel(vtx_layout, 100, 0.01, 100);
local ground_ref = res.AddModel("ground", ground_mdl);

local shader = hg.LoadPipelineProgramRefFromAssets("core/shader/default.hps", res, hg.GetForwardPipelineInfo());

local mat_light = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4(0, 0, 0), "uSpecularColor", hg.Vec4(0, 0, 0));
hg.SetMaterialValue(mat_light, "uSelfColor", hg.Vec4(1, 0.9, 0.75));
local mat_orb = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4(1, 1, 1), "uSpecularColor", hg.Vec4(1, 1, 1));
hg.SetMaterialValue(mat_orb, "uSelfColor", hg.Vec4(0, 0, 0));
local mat_ground = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4(1, 1, 1), "uSpecularColor", hg.Vec4(1, 1, 1));
hg.SetMaterialValue(mat_ground, "uSelfColor", hg.Vec4(0, 0, 0));

local scene = hg.Scene();

local cam = hg.CreateCamera(scene, hg.Mat4LookAt(hg.Vec3(5, 4, -7), hg.Vec3(0, 1.5, 0)), 0.01, 1000);
scene.SetCurrentCamera(cam);

local orb_node = hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(0, 1, 0)), orb_ref, [mat_orb]);
hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(0, 0, 0)), ground_ref, [mat_ground]);

local light_obj = scene.CreateObject(light_ref, [mat_light]);
local light_nodes = [];

for (local i = 0; i < 16; ++i) {
	local node = hg.CreatePointLight(scene, hg.Mat4.get_Identity(), 1.5, hg.Color(1, 0.85, 0.25, 1), hg.Color(1, 0.9, 0.5, 1));
	node.SetObject(light_obj);
	light_nodes.append(node);
}

local angle = 0.0;

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();
	angle += hg.time_to_sec_f(dt);

	for (local i = 0; i < light_nodes.len(); ++i) {
		local node = light_nodes[i];
		local a = angle + i * hg.Deg(15);
		node.GetTransform().SetPos(hg.Vec3(cos(a * -0.6) * sin(a) * 5, cos(a * 1.25) * 2 + 2.15, sin(a * 0.5) * cos(-a * 0.8) * 5));
	}

	foreach (node in light_nodes) {
		local priority = hg.Dist(orb_node.GetTransform().GetPos(), node.GetTransform().GetPos());
		node.GetLight().SetPriority(-priority);
	}

	scene.Update(dt);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.DestroyForwardPipeline(pipeline);
hg.RenderShutdown();
hg.DestroyWindow(win);

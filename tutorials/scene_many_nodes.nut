// Many dynamic objects

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Many dynamic objects", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

local pipeline = hg.CreateForwardPipeline(4096);
local res = hg.PipelineResources();

local vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

local sphere_mdl = hg.CreateSphereModel(vtx_layout, 0.1, 8, 16);
local sphere_ref = res.AddModel("sphere", sphere_mdl);
local ground_mdl = hg.CreateCubeModel(vtx_layout, 60, 0.001, 60);
local ground_ref = res.AddModel("ground", ground_mdl);

local shader = hg.LoadPipelineProgramRefFromFile("resources_compiled/core/shader/default.hps", res, hg.GetForwardPipelineInfo());

local sphere_mat = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4(1, 0, 0, 1), "uSpecularColor", hg.Vec4(1, 0.8, 0, 1));
local ground_mat = hg.CreateMaterial(shader, "uDiffuseColor", hg.Vec4(1, 1, 1, 1), "uSpecularColor", hg.Vec4(1, 1, 1, 1));

local scene = hg.Scene();
scene.canvas.color = hg.Color(0.1, 0.1, 0.1, 1);
scene.environment.ambient = hg.Color(0.1, 0.1, 0.1, 1);

local cam = hg.CreateCamera(scene, hg.TransformationMat4(hg.Vec3(15.5, 5, -6), hg.Vec3(0.4, -1.2, 0)), 0.01, 100);
scene.SetCurrentCamera(cam);

hg.CreateSpotLight(scene, hg.TransformationMat4(hg.Vec3(-8.8, 21.7, -8.8), hg.Deg3(60, 45, 0)), 0, hg.Deg(5), hg.Deg(30), hg.Color.get_White(), hg.Color.get_White(), 0, hg.LST_Map, 0.000005);
hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(0, 0, 0)), ground_ref, [ground_mat]);

local rows = [];
for (local z = -100; z <= 100; z += 2) {
	local row = [];
	for (local x = -100; x <= 100; x += 2) {
		local node = hg.CreateObject(scene, hg.TranslationMat4(hg.Vec3(x * 0.1, 0.1, z * 0.1)), sphere_ref, [sphere_mat]);
		row.append(node.GetTransform());
	}
	rows.append(row);
}

local angle = 0.0;

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();
	angle += hg.time_to_sec_f(dt);

	foreach (j, row in rows) {
		local row_y = cos(angle + j * 0.1);
		foreach (i, trs in row) {
			local pos = trs.GetPos();
			pos.y = 0.1 * (row_y * sin(angle + i * 0.1) * 6 + 6.5);
			trs.SetPos(pos);
		}
	}

	scene.Update(dt);

	hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
	hg.Frame();
	hg.UpdateWindow(win);

	collectgarbage();
}

hg.DestroyForwardPipeline(pipeline);
hg.RenderShutdown();
hg.DestroyWindow(win);

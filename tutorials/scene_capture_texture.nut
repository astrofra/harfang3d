// Toyota 2JZ-GTE Engine model by Serhii Denysenko (CGTrader: serhiidenysenko8256)
// URL : https://www.cgtrader.com/3d-models/vehicle/part/toyota-2jz-gte-engine-2932b715-2f42-4ecd-93ce-df9507c67ce8

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1024;
local res_y = 1024;
local tex_size = 1024;
local win = hg.RenderInit("Scene Capture Texture - Press SpaceBar to capture", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local frame_buffer = hg.CreateFrameBuffer(tex_size, tex_size, hg.TF_RGBA8, hg.TF_D24, 4, "framebuffer");
local tex_color = hg.GetColorTexture(frame_buffer);

local tex_color_ref = res.AddTexture("tex_rb", tex_color);
local tex_readback = hg.CreateTexture(tex_size, tex_size, "readback", hg.TF_ReadBack | hg.TF_BlitDestination, hg.TF_RGBA8);
local picture = hg.Picture(tex_size, tex_size, hg.PF_RGBA32);

local scene = hg.Scene();
hg.LoadSceneFromAssets("car_engine/engine.scn", scene, res, hg.GetForwardPipelineInfo());

local vtx_layout = hg.VertexLayoutPosFloatTexCoord0UInt8();
local plane_mdl = hg.CreatePlaneModel(vtx_layout, 1, 1, 1, 1);
local plane_ref = res.AddModel("plane", plane_mdl);

local plane_prg = hg.LoadProgramFromAssets("shaders/texture");

local frame = 0;
local state = "none";
local frame_count_capture = 0;

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();

	scene.Update(dt);

	local trs = scene.GetNode("engine_master").GetTransform();
	trs.SetRot(trs.GetRot() + hg.Vec3(0, hg.Deg(15) * hg.time_to_sec_f(dt), 0));

	local view_id = 0;
	local submit_result = hg.SubmitSceneToPipeline(view_id, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res, frame_buffer.handle);
	view_id = submit_result[0];

	hg.SetViewPerspective(view_id, 0, 0, res_x, res_y, hg.TranslationMat4(hg.Vec3(0, 0, -1.8)));

	local val_uniforms = [hg.MakeUniformSetValue("color", hg.Vec4(1, 1, 1, 1))];
	local tex_uniforms = [hg.MakeUniformSetTexture("s_tex", tex_color, 0)];
	hg.DrawModel(view_id, plane_mdl, plane_prg, val_uniforms, tex_uniforms, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(1.57079632679, 0, 3.14159265359)));

	if (hg.ReadKeyboard().Key(hg.K_Space) && state == "none") {
		state = "capture";
		local capture_result = hg.CaptureTexture(view_id, res, tex_color_ref, tex_readback, picture);
		frame_count_capture = capture_result[0];
		view_id = capture_result[1];
	} else if (state == "capture" && frame_count_capture <= frame) {
		hg.SavePNG(picture, "capture.png");
		state = "none";
	}

	frame = hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);

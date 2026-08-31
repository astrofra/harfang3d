// Draw scene to texture

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1024;
local res_y = 1024;
local win = hg.RenderInit("Draw Scene to Texture", res_x, res_y, hg.RF_VSync | hg.RF_MSAA8X);

hg.AddAssetsFolder("resources_compiled");

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local scene = hg.Scene();
hg.LoadSceneFromAssets("materials/materials.scn", scene, res, hg.GetForwardPipelineInfo());

local frame_buffer = hg.CreateFrameBuffer(512, 512, hg.TF_RGBA32F, hg.TF_D24, 4, "framebuffer");
local color = hg.GetColorTexture(frame_buffer);

local vtx_layout = hg.VertexLayoutPosFloatTexCoord0UInt8();
local cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
local cube_ref = res.AddModel("cube", cube_mdl);

local cube_prg = hg.LoadProgramFromAssets("shaders/texture");

local angle = 0.0;

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();
	angle += hg.time_to_sec_f(dt);

	scene.GetCurrentCamera().GetTransform().SetPos(hg.Vec3(0, 0, -(sin(angle) * 3 + 4)));
	scene.Update(dt);

	local view_id = 0;
	local submit_result = hg.SubmitSceneToPipeline(view_id, scene, hg.IntRect(0, 0, 512, 512), true, pipeline, res, frame_buffer.handle);
	view_id = submit_result[0];

	hg.SetViewPerspective(view_id, 0, 0, res_x, res_y, hg.TranslationMat4(hg.Vec3(0, 0, -1.8)));

	local val_uniforms = [hg.MakeUniformSetValue("color", hg.Vec4(1, 1, 1, 1))];
	local tex_uniforms = [hg.MakeUniformSetTexture("s_tex", color, 0)];
	hg.DrawModel(view_id, cube_mdl, cube_prg, val_uniforms, tex_uniforms, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(angle * 0.1, angle * 0.05, angle * 0.2)));

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.WindowSystemShutdown();

// Manually setup node physics

local hg = require("harfang");

hg.InputInit();
hg.WindowSystemInit();

local res_x = 1280;
local res_y = 720;
local win = hg.RenderInit("Harfang - Node Physics Setup", res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X);

local pipeline = hg.CreateForwardPipeline();
local res = hg.PipelineResources();

local vtx_mdl = hg.VertexLayoutPosFloatNormUInt8();

local cube_mdl = hg.CreateCubeModel(vtx_mdl, 1, 1, 1);
local cube_ref = res.AddModel("cube", cube_mdl);

local ground_mdl = hg.CreateCubeModel(vtx_mdl, 50, 0.01, 50);
local ground_ref = res.AddModel("ground", ground_mdl);

local prg_ref = hg.LoadPipelineProgramRefFromFile("resources_compiled/core/shader/default.hps", res, hg.GetForwardPipelineInfo());
local mat = hg.CreateMaterial(prg_ref, "uDiffuseColor", hg.Vec4(0.5, 0.5, 0.5, 1), "uSpecularColor", hg.Vec4(0.0, 0.0, 0.0, 0.1));

local scene = hg.Scene();

local cam = hg.CreateCamera(scene, hg.TransformationMat4(hg.Vec3(0, 1, -5), hg.Deg3(5, 0, 0)), 0.01, 1000);
scene.SetCurrentCamera(cam);

hg.CreatePointLight(scene, hg.TranslationMat4(hg.Vec3(6, 4, -6)), 0);
hg.CreatePhysicCube(scene, hg.Vec3(100, 0.02, 100), hg.TranslationMat4(hg.Vec3(0, -0.005, 0)), ground_ref, [mat], 0);

local clocks = hg.SceneClocks();

local cube_node = hg.CreateObject(scene, hg.TransformationMat4(hg.Vec3(0, 2.5, 0), hg.Vec3(0, 0, 0)), cube_ref, [mat]);

local rb = scene.CreateRigidBody();
rb.SetType(hg.RBT_Dynamic);

local collision = scene.CreateCollision();
collision.SetType(hg.CT_Cube);
collision.SetSize(hg.Vec3(1, 1, 1));
collision.SetMass(1);

cube_node.SetRigidBody(rb);
cube_node.SetCollision(0, collision);

local physics = hg.SceneBullet3Physics();
physics.SceneCreatePhysicsFromAssets(scene);

while (!hg.ReadKeyboard().Key(hg.K_Escape) && hg.IsWindowOpen(win)) {
	local dt = hg.TickClock();

	hg.SceneUpdateSystems(scene, clocks, dt, physics, hg.time_from_sec_f(1 / 60.0), 1);
	local submit_result = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);
	local view_id = submit_result[0];

	hg.Frame();
	hg.UpdateWindow(win);
}

hg.RenderShutdown();
hg.DestroyWindow(win);

hg.WindowSystemShutdown();
hg.InputShutdown();

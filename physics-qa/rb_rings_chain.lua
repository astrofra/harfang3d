hg = require("harfang")

hg.AddAssetsFolder('assets_compiled')

-- main window
hg.InputInit()
hg.WindowSystemInit()

res_x, res_y = 1280, 720
win = hg.RenderInit('Physics Test', res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X)

pipeline = hg.CreateForwardPipeline(2048)
res = hg.PipelineResources()

-- physics debug
vtx_line_layout = hg.VertexLayoutPosFloatColorUInt8()
line_shader = hg.LoadProgramFromAssets("shaders/pos_rgb")

-- create material
pbr_shader = hg.LoadPipelineProgramRefFromAssets('core/shader/pbr.hps', res, hg.GetForwardPipelineInfo())
mat_grey = hg.CreateMaterial(pbr_shader, 'uBaseOpacityColor', hg.Vec4(1, 1, 1), 'uOcclusionRoughnessMetalnessColor', hg.Vec4(1, 0.5, 0.05))

-- create models
vtx_layout = hg.VertexLayoutPosFloatNormUInt8()

-- ground
ground_size = hg.Vec3(50, 0.05, 50)
ground_ref = res:AddModel('ground', hg.CreateCubeModel(vtx_layout, ground_size.x, ground_size.y, ground_size.z))

-- setup the scene
scene = hg.Scene()
hg.LoadSceneFromAssets("ring_scene/rings.scn", scene, res, hg.GetForwardPipelineInfo())

cam_mat = hg.TransformationMat4(hg.Vec3(0, 20, -45.5) * 2.0, hg.Vec3(hg.Deg(15), 0, 0))
cam = hg.CreateCamera(scene, cam_mat, 0.01, 1000, hg.Deg(30))
view_matrix = hg.InverseFast(cam_mat)
c = cam:GetCamera()
projection_matrix = hg.ComputePerspectiveProjectionMatrix(c:GetZNear(), c:GetZFar(), hg.FovToZoomFactor(c:GetFov()), hg.Vec2(res_x / res_y, 1))

scene:SetCurrentCamera(cam)	

lgt = hg.CreateLinearLight(scene, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(hg.Deg(30), hg.Deg(30), 0)), hg.Color(1, 1, 1), hg.Color(1, 1, 1), 10, hg.LST_Map, 0.001, hg.Vec4(20, 34, 55, 70))


-- scene physics
physics = hg.SceneBullet3Physics()
physics:SceneCreatePhysicsFromAssets(scene)
physics_step = hg.time_from_sec_f(1 / 60)
dt_frame_step = hg.time_from_sec_f(1 / 60)

clocks = hg.SceneClocks()

-- description
hg.SetLogLevel(hg.LL_Normal)
print(">>> Description:\n>>> Dynamic rings chain, with a static ring of mass 0 at the top, and forces applied on the last link of the chain")

-- main loop
keyboard = hg.Keyboard()
bottom_ring = scene:GetNode("ring_dynamic_bottom")
bottom_ring_root = scene:GetNodeChildren(bottom_ring):at(0)
force = hg.Vec3(50,-30,0)
time_switch = 0

while not keyboard:Down(hg.K_Escape) and hg.IsWindowOpen(win) do
    keyboard:Update()

	dt = hg.TickClock()
    time_switch = time_switch + dt

    if hg.time_to_sec_f(time_switch) > 5 then
        if force == hg.Vec3(50,-30,0) then
            force = hg.Vec3(-50,-30,0)
        else
            force = hg.Vec3(50,-30,0)
        end
        time_switch = 0
    end

    view_id = 0
    hg.SceneUpdateSystems(scene, clocks, dt_frame_step, physics, physics_step, 3)
    view_id, pass_id = hg.SubmitSceneToPipeline(view_id, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res)
    physics:NodeAddForce(bottom_ring_root, force)

    -- -- Debug physics display
    -- hg.SetViewClear(view_id, 0, 0, 1.0, 0)
    -- hg.SetViewRect(view_id, 0, 0, res_x, res_y)
    -- hg.SetViewTransform(view_id, view_matrix, projection_matrix)
    -- rs = hg.ComputeRenderState(hg.BM_Opaque, hg.DT_Disabled, hg.FC_Disabled)
    -- physics:RenderCollision(view_id, vtx_line_layout, line_shader, rs, 0)

    hg.Frame()
    hg.UpdateWindow(win)
end

scene:Clear()
scene:GarbageCollect()

hg.RenderShutdown()
hg.DestroyWindow(win)

hg.WindowSystemShutdown()
hg.InputShutdown()
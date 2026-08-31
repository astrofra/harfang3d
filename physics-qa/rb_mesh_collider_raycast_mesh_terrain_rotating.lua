hg = require("harfang")

hg.AddAssetsFolder('assets_compiled')

-- main window
hg.InputInit()
hg.WindowSystemInit()

res_x, res_y = 1280, 720
win = hg.RenderInit('Physics Test', res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X)

pipeline = hg.CreateForwardPipeline()
res = hg.PipelineResources()

-- physics debug
vtx_line_layout = hg.VertexLayoutPosFloatColorUInt8()
line_shader = hg.LoadProgramFromAssets("shaders/pos_rgb")

-- setup the scene
scene = hg.Scene()

hg.LoadSceneFromAssets("island_chain/island_chain.scn", scene, res, hg.GetForwardPipelineInfo())

cam_mat = hg.TransformationMat4(hg.Vec3(10, 2, -2) * 2000.0, hg.Vec3(hg.Deg(25), hg.Deg(-65), 0))
cam = hg.CreateCamera(scene, cam_mat, 10.0, 40000)
view_matrix = hg.InverseFast(cam_mat)
c = cam:GetCamera()
projection_matrix = hg.ComputePerspectiveProjectionMatrix(c:GetZNear(), c:GetZFar(), hg.FovToZoomFactor(c:GetFov()),
    hg.Vec2(res_x / res_y, 1))

scene:SetCurrentCamera(cam)

-- scene physics
physics = hg.SceneBullet3Physics()
physics:SceneCreatePhysicsFromAssets(scene)
physics_step = hg.time_from_sec_f(1 / 60)
dt_frame_step = hg.time_from_sec_f(1 / 60)

clocks = hg.SceneClocks()

-- description
hg.SetLogLevel(hg.LL_Normal)
print(
    ">>> Description:\n>>> Create a mesh collider with a terrain collider from its .physics_bullet file and test the collisions with raycasts.\n" ..
    ">>> the 'Island chain` 3D model (65k triangles, 30x12Km) is a courtesy of World Machine.")

island_node = scene:GetNode("island_chain")
mesh_col = scene:CreateCollision()
mesh_col:SetType(hg.CT_Mesh)
mesh_col:SetCollisionResource("island_chain/island_chain.physics_bullet")
mesh_col:SetMass(0)
island_node:SetCollision(0, mesh_col)

-- create rigid body
rb = scene:CreateRigidBody()
rb:SetType(hg.RBT_Kinematic)
island_node:SetRigidBody(rb)
physics:NodeCreatePhysicsFromAssets(island_node)

-- main loop
keyboard = hg.Keyboard()
mouse = hg.Mouse()
cam_pos = cam:GetTransform():GetPos()
cam_rot = cam:GetTransform():GetRot()
vtx = hg.Vertices(vtx_line_layout, 2)
vid_scene_opaque = 0
local frame_count = 0

while not keyboard:Down(hg.K_Escape) and hg.IsWindowOpen(win) do
    keyboard:Update()
    mouse:Update()

    dt = hg.TickClock()

    island_node:GetTransform():SetRot(hg.Vec3(0.0,  math.pi * frame_count / 360.0, 0.0))
    -- Set world matrix (position / rotation)
    -- _pos = hg.Vec3(0.0, 0.0, 0.0)
    -- _rot = hg.Vec3(0.0,  math.pi * frame_count / 360.0, 0.0)
    -- island_node:GetTransform():SetWorld(hg.TransformationMat4(_pos, _rot))

    if keyboard:Down(hg.K_LShift) then
        s = 2000
    else
        s = 200
    end

    hg.FpsController(keyboard, mouse, cam_pos, cam_rot, s, dt)

    cam:GetTransform():SetPos(cam_pos)
    cam:GetTransform():SetRot(cam_rot)

    local xz_spread = 300.0

    for i = -60, 60 do
        for o = -15, 15 do
            start_pos = hg.Vec3(0 + i * xz_spread, 1000.0, 0 + o * xz_spread)
            end_pos = hg.Vec3(0 + i * xz_spread, -100.0, 0 + o * xz_spread)
            raycast_out = physics:RaycastFirstHit(scene, start_pos, end_pos)
            if raycast_out.node:IsValid() then
                vtx:Clear()
                vtx:Begin(0):SetPos(start_pos):SetColor0(hg.Color.Yellow):End()
                vtx:Begin(1):SetPos(raycast_out.P):SetColor0(hg.Color.Yellow):End()
                hg.DrawLines(vid_scene_opaque, vtx, line_shader) -- submit all lines in a single call
            else
                vtx:Clear()
                vtx:Begin(0):SetPos((start_pos + end_pos) * 0.5):SetColor0(hg.Color.Red):End()
                vtx:Begin(1):SetPos(end_pos):SetColor0(hg.Color.Red):End()
                hg.DrawLines(vid_scene_opaque, vtx, line_shader) -- submit all lines in a single call                
            end
        end
    end

    view_id = 0
    hg.SceneUpdateSystems(scene, clocks, dt_frame_step, physics, physics_step, 3)
    view_id, pass_id = hg.SubmitSceneToPipeline(view_id, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res)
    vid_scene_opaque = hg.GetSceneForwardPipelinePassViewId(pass_id, hg.SFPP_Opaque)

    -- -- Debug physics display
    -- hg.SetViewClear(view_id, 0, 0, 1.0, 0)
    -- hg.SetViewRect(view_id, 0, 0, res_x, res_y)
    -- cam_mat = cam:GetTransform():GetWorld()
    -- view_matrix = hg.InverseFast(cam_mat)
    -- c = cam:GetCamera()
    -- projection_matrix = hg.ComputePerspectiveProjectionMatrix(c:GetZNear(), c:GetZFar(), hg.FovToZoomFactor(c:GetFov()),
    --     hg.Vec2(res_x / res_y, 1))
    -- hg.SetViewTransform(view_id, view_matrix, projection_matrix)
    -- rs = hg.ComputeRenderState(hg.BM_Opaque, hg.DT_Disabled, hg.FC_Disabled)
    -- physics:RenderCollision(view_id, vtx_line_layout, line_shader, rs, 0)

    frame_count = frame_count + 1

    hg.Frame()
    hg.UpdateWindow(win)
end

scene:Clear()
scene:GarbageCollect()

hg.RenderShutdown()
hg.DestroyWindow(win)

hg.WindowSystemShutdown()
hg.InputShutdown()

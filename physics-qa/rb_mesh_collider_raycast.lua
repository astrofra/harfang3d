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

cam_mat = hg.TransformationMat4(hg.Vec3(0, 1.0, -2.15), hg.Vec3(hg.Deg(30), 0, 0))
cam = hg.CreateCamera(scene, cam_mat, 0.01, 1000)
view_matrix = hg.InverseFast(cam_mat)
c = cam:GetCamera()
projection_matrix = hg.ComputePerspectiveProjectionMatrix(c:GetZNear(), c:GetZFar(), hg.FovToZoomFactor(c:GetFov()),
    hg.Vec2(res_x / res_y, 1))

scene:SetCurrentCamera(cam)

lgt = hg.CreateLinearLight(scene, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(hg.Deg(30), hg.Deg(30), 0)),
    hg.Color(1, 1, 1), hg.Color(1, 1, 1), 10, hg.LST_Map, 0.0001, hg.Vec4(2, 4, 10, 16))

cube_instance, _ = hg.CreateInstanceFromAssets(scene, hg.TranslationMat4(hg.Vec3(0, 0, 0)), "plane_vertice_grid/plane_vertice_grid.scn",
    res, hg.GetForwardPipelineInfo())

-- scene physics
physics = hg.SceneBullet3Physics()
physics:SceneCreatePhysicsFromAssets(scene)
physics_step = hg.time_from_sec_f(1 / 60)
dt_frame_step = hg.time_from_sec_f(1 / 60)

clocks = hg.SceneClocks()

-- description
hg.SetLogLevel(hg.LL_Normal)
print(
    ">>> Description:\n>>> Create a mesh collider with a subdivided surface from a backend-neutral .physics resource and test it with raycasts.")

-- init mesh physics
plan_node = scene:GetNode("Plan")
mesh_col = scene:CreateCollision()
mesh_col:SetType(hg.CT_Mesh)
mesh_col:SetCollisionResource("plane_vertice_grid/Plan_38.physics")
mesh_col:SetMass(0)
plan_node:SetCollision(0, mesh_col)
-- create rigid body
rb = scene:CreateRigidBody()
rb:SetType(hg.RBT_Static)
rb:SetFriction(0.498)
rb:SetRollingFriction(0)
plan_node:SetRigidBody(rb)
physics:NodeCreatePhysicsFromAssets(plan_node)

-- main loop
keyboard = hg.Keyboard()
mouse = hg.Mouse()
cam_pos = cam:GetTransform():GetPos()
cam_rot = cam:GetTransform():GetRot()
vtx = hg.Vertices(vtx_line_layout, 2)
vid_scene_opaque = 0
local frame_count = 0
raycast_qa = os.getenv("HG_PHYSICS_QA_MODE") == "raycast_check"
raycast_qa_running = true

while raycast_qa_running and not keyboard:Down(hg.K_Escape) and hg.IsWindowOpen(win) do
    keyboard:Update()
    mouse:Update()

    dt = hg.TickClock()

    if keyboard:Down(hg.K_LShift) then
        s = 20
    else
        s = 8
    end

    hg.FpsController(keyboard, mouse, cam_pos, cam_rot, s, dt)

    cam:GetTransform():SetPos(cam_pos)
    cam:GetTransform():SetRot(cam_rot)

    -- for i = -8, 8 do
    --     for o = -8, 8 do
    --         start_pos = hg.Vec3(0 + i * 0.125, 0.1, 0 + o * 0.125)
    --         end_pos = hg.Vec3(0 + i * 0.125, -0.1, 0 + o * 0.125)
    --         raycast_out = physics:RaycastFirstHit(scene, start_pos, end_pos)
    --         if raycast_out.node:IsValid() then
    --             vtx:Clear()
    --             vtx:Begin(0):SetPos(start_pos):SetColor0(hg.Color.Green):End()
    --             vtx:Begin(1):SetPos(raycast_out.P):SetColor0(hg.Color.Green):End()
    --             hg.DrawLines(vid_scene_opaque, vtx, line_shader) -- submit all lines in a single call
    --         else
    --             vtx:Clear()
    --             vtx:Begin(0):SetPos(start_pos):SetColor0(hg.Color.Red):End()
    --             vtx:Begin(1):SetPos(end_pos):SetColor0(hg.Color.Red):End()
    --             hg.DrawLines(vid_scene_opaque, vtx, line_shader) -- submit all lines in a single call
    --         end
    --     end
    -- end

    local raycast_hits = 0
    local raycast_misses = 0
    local raycast_miss_coordinates = {}
    for i = -20, 20, 2 do
        for o = -20, 20, 2 do
            start_pos = hg.Vec3(0 + i * 0.05, 0.1, 0 + o * 0.05)
            end_pos = hg.Vec3(0 + i * 0.05, -0.1, 0 + o * 0.05)
            raycast_out = physics:RaycastFirstHit(scene, start_pos, end_pos)
            if raycast_out.node:IsValid() then
                raycast_hits = raycast_hits + 1
                vtx:Clear()
                vtx:Begin(0):SetPos(start_pos):SetColor0(hg.Color.Yellow):End()
                vtx:Begin(1):SetPos(raycast_out.P):SetColor0(hg.Color.Yellow):End()
                hg.DrawLines(vid_scene_opaque, vtx, line_shader) -- submit all lines in a single call
            else
                raycast_misses = raycast_misses + 1
                if raycast_qa then
                    table.insert(raycast_miss_coordinates, string.format("(%.2f,%.2f)", start_pos.x, start_pos.z))
                end
                vtx:Clear()
                vtx:Begin(0):SetPos(start_pos):SetColor0(hg.Color.Red):End()
                vtx:Begin(1):SetPos(end_pos):SetColor0(hg.Color.Red):End()
                hg.DrawLines(vid_scene_opaque, vtx, line_shader) -- submit all lines in a single call
            end
        end
    end

    view_id = 0
    hg.SceneUpdateSystems(scene, clocks, dt_frame_step, physics, physics_step, 3)
    view_id, pass_id = hg.SubmitSceneToPipeline(view_id, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res)
    vid_scene_opaque = hg.GetSceneForwardPipelinePassViewId(pass_id, hg.SFPP_Opaque)

    -- Debug physics display
    hg.SetViewClear(view_id, 0, 0, 1.0, 0)
    hg.SetViewRect(view_id, 0, 0, res_x, res_y)
    cam_mat = cam:GetTransform():GetWorld()
    view_matrix = hg.InverseFast(cam_mat)
    c = cam:GetCamera()
    projection_matrix = hg.ComputePerspectiveProjectionMatrix(c:GetZNear(), c:GetZFar(), hg.FovToZoomFactor(c:GetFov()),
        hg.Vec2(res_x / res_y, 1))
    hg.SetViewTransform(view_id, view_matrix, projection_matrix)
    rs = hg.ComputeRenderState(hg.BM_Opaque, hg.DT_Disabled, hg.FC_Disabled)
    physics:RenderCollision(view_id, vtx_line_layout, line_shader, rs, 0)

    frame_count = frame_count + 1

    hg.Frame()
    hg.UpdateWindow(win)

    if raycast_qa then
        -- Bullet's cooked triangle mesh treats the outer border as open: the
        -- 19x19 interior rays hit and the 80 perimeter rays miss.
        assert(raycast_hits == 361, string.format("Mesh raycast QA expected 361 hits, got %d hits and %d misses at %s", raycast_hits,
            raycast_misses, table.concat(raycast_miss_coordinates, ", ")))
        assert(raycast_misses == 80, string.format("Mesh raycast QA expected 80 perimeter misses, got %d", raycast_misses))
        print(string.format("Mesh raycast QA passed: hits=%d, misses=%d", raycast_hits, raycast_misses))
        raycast_qa_running = false
    end
end

scene:Clear()
scene:GarbageCollect()

hg.RenderShutdown()
hg.DestroyWindow(win)

hg.WindowSystemShutdown()
hg.InputShutdown()

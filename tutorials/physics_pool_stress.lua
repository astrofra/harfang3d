-- Auto-stress variant of physics_pool_of_objects.lua, no keyboard needed.
-- Spawns up to TARGET_COUNT physics nodes in batches of BATCH_PER_FRAME,
-- then settles for SETTLE_FRAMES and exits. Prints per-second status to stdout.

hg = require("harfang")
hg.InputInit()
hg.WindowSystemInit()

res_x, res_y = 1280, 720
win = hg.RenderInit('Harfang - Physics Pool (stress)', res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X)

hg.AddAssetsFolder('resources_compiled')

pipeline = hg.CreateForwardPipeline()
res = hg.PipelineResources()

vtx_layout = hg.VertexLayoutPosFloatNormUInt8()
sphere_mdl = hg.CreateSphereModel(vtx_layout, 0.5, 12, 24)
sphere_ref = res:AddModel('sphere', sphere_mdl)
cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1)
cube_ref = res:AddModel('cube', cube_mdl)

prg_ref = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', res, hg.GetForwardPipelineInfo())

local function create_material(diffuse, specular, self)
    local mat = hg.CreateMaterial(prg_ref, 'uDiffuseColor', diffuse, 'uSpecularColor', specular)
    hg.SetMaterialValue(mat, 'uSelfColor', self)
    return mat
end

local mat_ground  = create_material(hg.Vec4(0.5, 0.5, 0.5), hg.Vec4(0.1, 0.1, 0.1), hg.Vec4(0, 0, 0))
local mat_walls   = create_material(hg.Vec4(0.5, 0.5, 0.5), hg.Vec4(0.1, 0.1, 0.1), hg.Vec4(0, 0, 0))
local mat_objects = create_material(hg.Vec4(0.5, 0.5, 0.5), hg.Vec4(1, 1, 1), hg.Vec4(0, 0, 0))

scene = hg.Scene()
scene.canvas.color = hg.ColorI(22, 56, 76)
scene.environment.fog_color = scene.canvas.color
scene.environment.fog_near = 20
scene.environment.fog_far = 80

cam_mtx = hg.TransformationMat4(hg.Vec3(0, 20, -30), hg.Deg3(30, 0, 0))
cam = hg.CreateCamera(scene, cam_mtx, 0.01, 5000)
scene:SetCurrentCamera(cam)

hg.CreateLinearLight(scene, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Deg3(30, 59, 0)), hg.Color(1, 0.8, 0.7), hg.Color(1, 0.8, 0.7), 10, hg.LST_Map, 0.002, hg.Vec4(50, 100, 200, 400))
hg.CreatePointLight(scene, hg.TranslationMat4(hg.Vec3(0, 10, 10)), 100, hg.ColorI(94, 155, 228), hg.ColorI(94, 255, 228))

local mdl_ref = res:AddModel('ground', hg.CreateCubeModel(vtx_layout, 100, 1, 100))
hg.CreatePhysicCube(scene, hg.Vec3(30, 1, 30), hg.TranslationMat4(hg.Vec3(0, -0.5, 0)), mdl_ref, {mat_ground}, 0)
mdl_ref = res:AddModel('wall', hg.CreateCubeModel(vtx_layout, 1, 11, 32))
hg.CreatePhysicCube(scene, hg.Vec3(1, 11, 32), hg.TranslationMat4(hg.Vec3(-15.5, -0.5, 0)), mdl_ref, {mat_walls}, 0)
hg.CreatePhysicCube(scene, hg.Vec3(1, 11, 32), hg.TranslationMat4(hg.Vec3(15.5, -0.5, 0)), mdl_ref, {mat_walls}, 0)
mdl_ref = res:AddModel('wall2', hg.CreateCubeModel(vtx_layout, 32, 11, 1))
hg.CreatePhysicCube(scene, hg.Vec3(32, 11, 1), hg.TranslationMat4(hg.Vec3(0, -0.5, -15.5)), mdl_ref, {mat_walls}, 0)
hg.CreatePhysicCube(scene, hg.Vec3(32, 11, 1), hg.TranslationMat4(hg.Vec3(0, -0.5, 15.5)), mdl_ref, {mat_walls}, 0)

clocks = hg.SceneClocks()
physics = hg.SceneBullet3Physics()
physics:SceneCreatePhysicsFromAssets(scene)

font = hg.LoadFontFromAssets('font/default.ttf', 32)
font_program = hg.LoadProgramFromAssets('core/shader/font.vsb', 'core/shader/font.fsb')
local text_uniform_values = {hg.MakeUniformSetValue('u_color', hg.Vec4(1, 1, 0.5))}
local text_render_state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Always, hg.FC_Disabled)

-- ----- stress params (override via env) -----
local TARGET_COUNT    = tonumber(os.getenv('STRESS_TARGET')) or 1500
local BATCH_PER_FRAME = tonumber(os.getenv('STRESS_BATCH'))  or 12
local MAX_FRAMES      = tonumber(os.getenv('STRESS_FRAMES')) or 3600  -- ~60s @ 60fps cap
local SETTLE_FRAMES   = tonumber(os.getenv('STRESS_SETTLE')) or 600
print(string.format("[stress] target=%d batch=%d max_frames=%d settle=%d", TARGET_COUNT, BATCH_PER_FRAME, MAX_FRAMES, SETTLE_FRAMES))
io.stdout:setvbuf('line')

local physic_nodes = {}
local frame = 0
local settle_left = 0
local t0 = os.time()
local last_print = 0

while hg.IsWindowOpen(win) and frame < MAX_FRAMES do
    frame = frame + 1

    if #physic_nodes < TARGET_COUNT then
        for i = 1, BATCH_PER_FRAME do
            if #physic_nodes >= TARGET_COUNT then break end
            hg.SetMaterialValue(mat_objects, 'uDiffuseColor', hg.RandomVec4(0, 1))
            local node
            if hg.FRand() > 0.5 then
                node = hg.CreatePhysicCube(scene, hg.Vec3.One, hg.TranslationMat4(hg.RandomVec3(hg.Vec3(-10, 18, -10), hg.Vec3(10, 18, 10))), cube_ref, {mat_objects}, 1)
            else
                node = hg.CreatePhysicSphere(scene, 0.5, hg.TranslationMat4(hg.RandomVec3(hg.Vec3(-10, 18, -10), hg.Vec3(10, 18, 10))), sphere_ref, {mat_objects}, 1)
            end
            physics:NodeCreatePhysicsFromAssets(node)
            table.insert(physic_nodes, node)
        end
    else
        settle_left = settle_left + 1
        if settle_left >= SETTLE_FRAMES then break end
    end

    local now = os.time()
    if now - last_print >= 1 then
        last_print = now
        print(string.format("[stress] t=%2ds frame=%4d objects=%4d", now - t0, frame, #physic_nodes))
    end

    local dt = hg.TickClock()
    hg.SceneUpdateSystems(scene, clocks, dt, physics, hg.time_from_sec_f(1 / 60), 4)
    local view_id = hg.SubmitSceneToPipeline(0, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res)

    hg.SetView2D(view_id, 0, 0, res_x, res_y, -1, 1, hg.CF_Depth, hg.Color.Black, 1, 0)
    hg.DrawText(view_id, font, string.format('STRESS: %d / %d', #physic_nodes, TARGET_COUNT), font_program, 'u_tex', 0, hg.Mat4.Identity, hg.Vec3(460, res_y - 60, 0), hg.DTHA_Left, hg.DTVA_Bottom, text_uniform_values, {}, text_render_state)
    hg.DrawText(view_id, font, string.format('frame %d', frame), font_program, 'u_tex', 0, hg.Mat4.Identity, hg.Vec3(res_x - 200, res_y - 60, 0), hg.DTHA_Left, hg.DTVA_Bottom, text_uniform_values, {}, text_render_state)

    hg.Frame()
    hg.UpdateWindow(win)
end

print(string.format("[stress] DONE frame=%d objects=%d elapsed=%ds", frame, #physic_nodes, os.time() - t0))
hg.RenderShutdown()
hg.DestroyWindow(win)
hg.WindowSystemShutdown()
hg.InputShutdown()

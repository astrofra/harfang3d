-- Physics Impulse

hg = require("harfang")
qa_dump = require("physics_qa_dump")

-- import harfang as hg
-- from time import sleep
-- from random import uniform

hg.InputInit()
hg.WindowSystemInit()

res_x, res_y = 1280, 720
win = hg.RenderInit('LEFT CUBE = impulse via renderloop | RIGHT CUBE = impulse via callback', res_x, res_y, hg.RF_VSync | hg.RF_MSAA4X)

pipeline = hg.CreateForwardPipeline()
res = hg.PipelineResources()

-- physics debug
vtx_line_layout = hg.VertexLayoutPosFloatColorUInt8()
line_shader = hg.LoadProgramFromFile("assets_compiled/shaders/pos_rgb")

-- create models
vtx_layout = hg.VertexLayoutPosFloatNormUInt8()

cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1)
cube_ref = res:AddModel('cube', cube_mdl)

ground_mdl = hg.CreateCubeModel(vtx_layout, 50, 0.01, 50)
ground_ref = res:AddModel('ground', ground_mdl)

-- create material
prg_ref = hg.LoadPipelineProgramRefFromFile('assets_compiled/core/shader/pbr.hps', res, hg.GetForwardPipelineInfo())
mat = hg.CreateMaterial(prg_ref, 'uBaseOpacityColor', hg.Vec4(1, 1, 1), 'uOcclusionRoughnessMetalnessColor', hg.Vec4(1, 0.5, 0.05))

-- setup scene
scene = hg.Scene()

cam_mat = hg.TransformationMat4(hg.Vec3(0, 4.0, -8), hg.Vec3(hg.Deg(10), 0, 0))
cam = hg.CreateCamera(scene, cam_mat, 0.01, 1000)
scene:SetCurrentCamera(cam)
view_matrix = hg.InverseFast(cam_mat)
c = cam:GetCamera()
projection_matrix = hg.ComputePerspectiveProjectionMatrix(c:GetZNear(), c:GetZFar(), hg.FovToZoomFactor(c:GetFov()), hg.Vec2(res_x / res_y, 1))

lgt = hg.CreateLinearLight(scene, hg.TransformationMat4(hg.Vec3(0, 0, 0), hg.Vec3(hg.Deg(30), hg.Deg(59), 0)), hg.Color(1, 1, 1), hg.Color(1, 1, 1), 10, hg.LST_Map, 0.002, hg.Vec4(2, 4, 10, 16))

cube_node_render_pos = hg.Vec3(-1, 2.0, 0)
cube_node_callback_pos = hg.Vec3(1, 2.0, 0)

cube_node_render = hg.CreatePhysicCube(scene, hg.Vec3(1, 1, 1), hg.TranslationMat4(cube_node_render_pos), cube_ref, {mat}, 2)
cube_node_callback = hg.CreatePhysicCube(scene, hg.Vec3(1, 1, 1), hg.TranslationMat4(cube_node_callback_pos), cube_ref, {mat}, 2)
ground_node = hg.CreatePhysicCube(scene, hg.Vec3(100, 0.02, 100), hg.TranslationMat4(hg.Vec3(0, -0.005, 0)), ground_ref, {mat}, 0)

clocks = hg.SceneClocks()

-- scene physics
scene:ReadyWorldMatrices()
scene:ComputeWorldMatrices()
physics = hg.SceneBullet3Physics()
physics:SceneCreatePhysicsFromAssets(scene)
physics_step = hg.time_from_sec_f(1 / 60)
dump = qa_dump.Create("rb_dynamic_impulse_callback", physics_step, {cube_node_render, cube_node_callback})
if dump.enabled then
	math.randomseed(1729)
end

-- main loop
keyboard = hg.Keyboard()

function randomFloat(lower, greater) 
    return lower + math.random()  * (greater - lower);
end


function draw_line(pos_a, pos_b, line_color, vid, vtx_line_layout, line_shader)
	vtx = hg.Vertices(vtx_line_layout, 2)
	vtx:Begin(0):SetPos(pos_a):SetColor0(line_color):End()
	vtx:Begin(1):SetPos(pos_b):SetColor0(line_color):End()
	hg.DrawLines(vid, vtx, line_shader)
end

function sleep(n)
	if n > 0 then os.execute("ping -n " .. tonumber(n+1) .. " localhost > NUL") end
  end

function impulse(ph, node, dt, target_pos) 
	cur_velocity = ph:NodeGetLinearVelocity(node)
	-- Pre-tick callbacks run while SceneUpdateSystems has invalidated the
	-- cached world matrices. Compute the node world directly in that phase.
	vel_to_target = target_pos - hg.GetT(node:ComputeWorld())
	vel_to_target = vel_to_target - cur_velocity
	ph:NodeAddImpulse(node, vel_to_target)
	ph:NodeWake(node)
end

callback_count = 0

function foo(ph, dt)
	callback_count = callback_count + 1
	impulse(ph, cube_node_callback, dt, cube_node_callback_pos)
end

function check_impulse_parity(ph)
	render_error = hg.GetT(cube_node_render:ComputeWorld()) - cube_node_render_pos
	callback_error = hg.GetT(cube_node_callback:ComputeWorld()) - cube_node_callback_pos
	position_error = hg.Len(render_error - callback_error)
	velocity_error = hg.Len(ph:NodeGetLinearVelocity(cube_node_render) - ph:NodeGetLinearVelocity(cube_node_callback))
	if position_error > 0.0001 or velocity_error > 0.0001 then
		error(string.format("Pre-tick impulse mismatch: position %.9g, velocity %.9g", position_error, velocity_error))
	end
end

physics:SetPreTickCallback(foo)

_ofs = 0.75
pos_timer = hg.time_from_sec_f(0.0)
physics_frame_count = 0

while not keyboard:Down(hg.K_Escape) and hg.IsWindowOpen(win) and not dump.complete do
	keyboard:Update()

	dt = dump.enabled and physics_step or hg.TickClock()
	view_id = 0
	lines = {}

	pos_timer = pos_timer + dt

	if pos_timer > hg.time_from_sec_f(5.0) then
		pos_timer = hg.time_from_sec_f(0.0)
		cube_node_render_pos.y = randomFloat(1.0, 5.0)
		cube_node_callback_pos.y = cube_node_render_pos.y
	end

	table.insert(lines, {cube_node_render_pos + hg.Vec3(_ofs,0,0), cube_node_render_pos - hg.Vec3(_ofs,0,0), hg.Color.Red})
	table.insert(lines, {cube_node_render_pos + hg.Vec3(0,_ofs,0), cube_node_render_pos - hg.Vec3(0,_ofs,0), hg.Color.Red})
	table.insert(lines, {cube_node_render_pos + hg.Vec3(0,0,_ofs), cube_node_render_pos - hg.Vec3(0,0,_ofs), hg.Color.Red})

	table.insert(lines, {cube_node_callback_pos + hg.Vec3(_ofs,0,0), cube_node_callback_pos - hg.Vec3(_ofs,0,0), hg.Color.Red})
	table.insert(lines, {cube_node_callback_pos + hg.Vec3(0,_ofs,0), cube_node_callback_pos - hg.Vec3(0,_ofs,0), hg.Color.Red})
	table.insert(lines, {cube_node_callback_pos + hg.Vec3(0,0,_ofs), cube_node_callback_pos - hg.Vec3(0,0,_ofs), hg.Color.Red})

	impulse(physics, cube_node_render, dt, cube_node_render_pos)

	hg.SceneUpdateSystems(scene, clocks, dt, physics, physics_step, 8)
	physics_frame_count = physics_frame_count + 1
	if dump.enabled then
		check_impulse_parity(physics)
	end
	qa_dump.Capture(dump, physics)
	view_id, pass_id = hg.SubmitSceneToPipeline(view_id, scene, hg.IntRect(0, 0, res_x, res_y), true, pipeline, res)

	-- debug draw lines
	opaque_view_id = hg.GetSceneForwardPipelinePassViewId(pass_id, hg.SFPP_Opaque)
	for i = 1, #lines do
		draw_line(lines[i][1], lines[i][2], lines[i][3], opaque_view_id, vtx_line_layout, line_shader)
	end

	-- Debug physics display
	hg.SetViewClear(view_id, 0, 0, 1.0, 0)
	hg.SetViewRect(view_id, 0, 0, res_x, res_y)
	hg.SetViewTransform(view_id, view_matrix, projection_matrix)
	rs = hg.ComputeRenderState(hg.BM_Opaque, hg.DT_Disabled, hg.FC_Disabled)
	physics:RenderCollision(view_id, vtx_line_layout, line_shader, rs, 0)

	hg.Frame()
	hg.UpdateWindow(win)

	if not dump.enabled then
		sleep(randomFloat(0.0, 0.05))
	end
end

qa_dump.Close(dump)
if dump.enabled and callback_count ~= physics_frame_count then
	error(string.format("Pre-tick callback count mismatch: got %d calls for %d physics frames", callback_count, physics_frame_count))
end

hg.RenderShutdown()
hg.DestroyWindow(win)

hg.WindowSystemShutdown()
hg.InputShutdown()

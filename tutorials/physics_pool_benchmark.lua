-- Deterministic benchmark for the physics_pool_of_objects workload.
--
-- Run this script with the matching Bullet or Tau HG Lua package. Example:
--
--   set BENCH_BACKEND=tau
--   set BENCH_BODIES=1500
--   set BENCH_LAYOUT=pool
--   set BENCH_OUTPUT=physics_pool_tau.jsonl
--   ..\..\install\tau\hg_lua\lua.exe physics_pool_benchmark.lua
--
-- Set HG_TAU_PROFILE=1 for aggregate Tau phase timings. Profiled results are
-- marked in JSON and should be used for attribution, not backend comparison.
-- Set HG_TAU_ISLAND_DIAGNOSTICS=1 to emit a machine-readable island workload
-- snapshot every 60 complete Tau substeps. Diagnostic runs perturb timings.
-- BENCH_LAYOUT=spread places bodies on a deterministic non-overlapping grid
-- to expose broad-phase scaling independently from dense-pile contact growth.
-- BENCH_MODE selects how much of the frame is measured:
--   physics            physics backend only, with bare scene nodes;
--   scene              physics plus SceneUpdateSystems, without rendering;
--   no_physics         SceneUpdateSystems with bare nodes and no physics;
--   render             scene, physics, forward rendering, and presentation;
--   render_no_physics  the same rendered scene without a physics backend.
-- Render modes use a 1280x720 forward pipeline with 4x MSAA and no RF_VSync.
-- The untimed settling transition skips GPU submission, then the complete
-- render path is warmed again before the settled measurement window.
-- BENCH_PHASE=settled is a fixed BENCH_SETTLE-step cooldown; it does not
-- imply that every body is asleep. BENCH_PHASE=all_sleeping waits for that
-- observable backend state, up to BENCH_SLEEP_MAX steps.

local hg = require("harfang")

local function env_string(name, default)
	local value = os.getenv(name)
	if value == nil or value == "" then
		return default
	end
	return value
end

local function env_number(name, default)
	local value = os.getenv(name)
	if value == nil or value == "" then
		return default
	end
	local number = tonumber(value)
	assert(number ~= nil, string.format("%s must be numeric, got '%s'", name, value))
	return number
end

local function env_flag(name)
	local value = os.getenv(name)
	return value ~= nil and value ~= "" and value ~= "0"
end

local function json_string(value)
	value = tostring(value)
	value = value:gsub("\\", "\\\\")
	value = value:gsub('"', '\\"')
	value = value:gsub("\n", "\\n")
	value = value:gsub("\r", "\\r")
	value = value:gsub("\t", "\\t")
	return '"' .. value .. '"'
end

local backend = env_string("BENCH_BACKEND", "")
assert(backend == "bullet" or backend == "tau", "BENCH_BACKEND must be 'bullet' or 'tau'")

local tau_backend_available = hg.SceneTauPhysics ~= nil
if backend == "tau" then
	assert(tau_backend_available, "BENCH_BACKEND=tau requires a Tau-enabled HG Lua package")
else
	assert(not tau_backend_available, "BENCH_BACKEND=bullet requires a Bullet-enabled HG Lua package")
end

local mode = env_string("BENCH_MODE", "physics")
assert(mode == "physics" or mode == "scene" or mode == "no_physics" or mode == "render" or mode == "render_no_physics",
	"BENCH_MODE must be 'physics', 'scene', 'no_physics', 'render', or 'render_no_physics'")
local render_enabled = mode == "render" or mode == "render_no_physics"
local physics_enabled = mode ~= "no_physics" and mode ~= "render_no_physics"

local phase_selection = env_string("BENCH_PHASE", "both")
assert(phase_selection == "active" or phase_selection == "settled" or phase_selection == "all_sleeping" or phase_selection == "both",
	"BENCH_PHASE must be 'active', 'settled', 'all_sleeping', or 'both'")
assert(phase_selection ~= "all_sleeping" or physics_enabled,
	"BENCH_PHASE=all_sleeping requires a physics-enabled BENCH_MODE")

local shape_mix = env_string("BENCH_SHAPES", "mixed")
assert(shape_mix == "mixed" or shape_mix == "cube" or shape_mix == "sphere",
	"BENCH_SHAPES must be 'mixed', 'cube', or 'sphere'")

local layout = env_string("BENCH_LAYOUT", "pool")
assert(layout == "pool" or layout == "spread", "BENCH_LAYOUT must be 'pool' or 'spread'")

local target_count = math.floor(env_number("BENCH_BODIES", 1500))
local batch_size = math.floor(env_number("BENCH_BATCH", 7))
local sample_steps = math.floor(env_number("BENCH_SAMPLES", 120))
local warmup_steps = math.floor(env_number("BENCH_WARMUP", 10))
local settle_steps = math.floor(env_number("BENCH_SETTLE", 600))
local sleep_max_steps = math.floor(env_number("BENCH_SLEEP_MAX", 7200))
local sleep_poll_steps = math.floor(env_number("BENCH_SLEEP_POLL", 60))
local repetitions = math.floor(env_number("BENCH_REPETITIONS", 5))
local base_seed = math.floor(env_number("BENCH_SEED", 0x544155))
local fixed_step = hg.time_from_sec_f(1 / 60)
local output_path = env_string("BENCH_OUTPUT", string.format("physics_pool_%s.jsonl", backend))
local append_output = env_flag("BENCH_APPEND")
local profiling = env_flag("HG_TAU_PROFILE")
local contact_diagnostics = env_flag("HG_TAU_CONTACT_DIAGNOSTICS")
local island_diagnostics = env_flag("HG_TAU_ISLAND_DIAGNOSTICS")
local diagnostics_enabled = contact_diagnostics or island_diagnostics

assert(target_count > 0, "BENCH_BODIES must be positive")
assert(batch_size > 0, "BENCH_BATCH must be positive")
assert(sample_steps > 0, "BENCH_SAMPLES must be positive")
assert(warmup_steps >= 0, "BENCH_WARMUP must not be negative")
assert(settle_steps >= 0, "BENCH_SETTLE must not be negative")
assert(sleep_max_steps >= 0, "BENCH_SLEEP_MAX must not be negative")
assert(sleep_poll_steps > 0, "BENCH_SLEEP_POLL must be positive")
assert(repetitions > 0, "BENCH_REPETITIONS must be positive")

if profiling and backend ~= "tau" then
	print("[benchmark] warning: HG_TAU_PROFILE is enabled for a Bullet run")
end
if diagnostics_enabled and backend ~= "tau" then
	print("[benchmark] warning: Tau diagnostics are enabled for a Bullet run")
end
if diagnostics_enabled then
	print("[benchmark] warning: diagnostics collection/logging perturbs timings; use this run for workload attribution only")
end

local render_context = nil
if render_enabled then
	hg.AddAssetsFolder("resources_compiled")
	hg.InputInit()
	hg.WindowSystemInit()
	local width, height = 1280, 720
	local window = hg.RenderInit("Harfang - Physics Pool Benchmark", width, height, hg.RF_MSAA4X)
	local pipeline = hg.CreateForwardPipeline()
	local resources = hg.PipelineResources()
	local vertex_layout = hg.VertexLayoutPosFloatNormUInt8()
	local program = hg.LoadPipelineProgramRefFromAssets("core/shader/default.hps", resources, hg.GetForwardPipelineInfo())
	local material = hg.CreateMaterial(program, "uDiffuseColor", hg.Vec4(0.5, 0.5, 0.5), "uSpecularColor", hg.Vec4(1, 1, 1))
	hg.SetMaterialValue(material, "uSelfColor", hg.Vec4(0, 0, 0))
	render_context = {
		width = width,
		height = height,
		window = window,
		pipeline = pipeline,
		resources = resources,
		vertex_layout = vertex_layout,
		material = material,
		cube_models = {},
		sphere_model = resources:AddModel("benchmark_sphere", hg.CreateSphereModel(vertex_layout, 0.5, 12, 24)),
	}
end

local output, output_error = io.open(output_path, append_output and "a" or "w")
assert(output ~= nil, string.format("failed to open BENCH_OUTPUT '%s': %s", output_path, output_error or "unknown error"))
output:setvbuf("line")

local function make_rng(seed)
	local state = seed % 4294967296
	return function()
		state = (1664525 * state + 1013904223) % 4294967296
		return state / 4294967296
	end
end

local function get_render_model(collision_type, size)
	if collision_type == hg.CT_Sphere then
		return render_context.sphere_model
	end
	local key = string.format("%.3f_%.3f_%.3f", size.x, size.y, size.z)
	local model = render_context.cube_models[key]
	if model == nil then
		model = render_context.resources:AddModel("benchmark_cube_" .. key,
			hg.CreateCubeModel(render_context.vertex_layout, size.x, size.y, size.z))
		render_context.cube_models[key] = model
	end
	return model
end

local function create_collision_body(scene, collision_type, position, size, radius, body_type, mass)
	local node
	if render_context ~= nil then
		node = hg.CreateObject(scene, hg.TranslationMat4(position), get_render_model(collision_type, size), {render_context.material})
	else
		node = scene:CreateNode()
		node:SetTransform(scene:CreateTransform(position))
	end

	local rigid_body = scene:CreateRigidBody()
	rigid_body:SetType(body_type)
	node:SetRigidBody(rigid_body)

	local collision = scene:CreateCollision()
	collision:SetType(collision_type)
	collision:SetMass(mass)
	if collision_type == hg.CT_Cube then
		collision:SetSize(size)
	else
		collision:SetRadius(radius)
	end
	node:SetCollision(0, collision)
	return node
end

local function create_container(scene, half_extent)
	local span = half_extent * 2
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(0, -0.5, 0), hg.Vec3(span, 1, span), 0, hg.RBT_Static, 0)
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(-half_extent - 0.5, -0.5, 0), hg.Vec3(1, 11, span + 2), 0, hg.RBT_Static, 0)
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(half_extent + 0.5, -0.5, 0), hg.Vec3(1, 11, span + 2), 0, hg.RBT_Static, 0)
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(0, -0.5, -half_extent - 0.5), hg.Vec3(span + 2, 11, 1), 0, hg.RBT_Static, 0)
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(0, -0.5, half_extent + 0.5), hg.Vec3(span + 2, 11, 1), 0, hg.RBT_Static, 0)
end

local function create_dynamic_body(scene, rng, body_index, spread_columns, spread_rows)
	local collision_type
	if shape_mix == "cube" then
		collision_type = hg.CT_Cube
	elseif shape_mix == "sphere" then
		collision_type = hg.CT_Sphere
	else
		collision_type = rng() > 0.5 and hg.CT_Cube or hg.CT_Sphere
	end

	local position
	if layout == "spread" then
		local spacing = 3
		local column = body_index % spread_columns
		local row = math.floor(body_index / spread_columns)
		position = hg.Vec3((column - (spread_columns - 1) * 0.5) * spacing, 18,
			(row - (spread_rows - 1) * 0.5) * spacing)
	else
		position = hg.Vec3(-10 + rng() * 20, 18, -10 + rng() * 20)
	end
	return create_collision_body(scene, collision_type, position, hg.Vec3(1, 1, 1), 0.5, hg.RBT_Dynamic, 1)
end

local function make_world(seed)
	local world = {
		scene = hg.Scene(),
		clocks = hg.SceneClocks(),
		physics = nil,
		nodes = {},
	}
	local spread_columns = math.ceil(math.sqrt(target_count))
	local spread_rows = math.ceil(target_count / spread_columns)
	local container_half_extent = layout == "spread" and (math.max(spread_columns, spread_rows) * 1.5 + 2) or 15
	if render_context ~= nil then
		world.scene.canvas.color = hg.ColorI(22, 56, 76)
		world.scene.environment.fog_color = world.scene.canvas.color
		world.scene.environment.fog_near = 20
		world.scene.environment.fog_far = 80
		local camera_matrix = hg.TransformationMat4(hg.Vec3(0, 20, -30), hg.Deg3(30, 0, 0))
		local camera = hg.CreateCamera(world.scene, camera_matrix, 0.01, 5000)
		world.scene:SetCurrentCamera(camera)
		hg.CreateLinearLight(world.scene, hg.TransformationMat4(hg.Vec3.Zero, hg.Deg3(30, 59, 0)),
			hg.Color(1, 0.8, 0.7), hg.Color(1, 0.8, 0.7), 10, hg.LST_Map, 0.002, hg.Vec4(50, 100, 200, 400))
		hg.CreatePointLight(world.scene, hg.TranslationMat4(hg.Vec3(0, 10, 10)), 100,
			hg.ColorI(94, 155, 228), hg.ColorI(94, 255, 228))
	end
	create_container(world.scene, container_half_extent)

	if physics_enabled then
		world.physics = hg.ScenePhysics()
		world.physics:SceneCreatePhysicsFromAssets(world.scene)
	end

	local rng = make_rng(seed)
	local created = 0
	while created < target_count do
		local count_this_batch = math.min(batch_size, target_count - created)
		for offset = 0, count_this_batch - 1 do
			local node = create_dynamic_body(world.scene, rng, created + offset, spread_columns, spread_rows)
			world.nodes[#world.nodes + 1] = node
			if world.physics ~= nil then
				world.physics:NodeCreatePhysicsFromAssets(node)
			end
		end
		created = created + count_this_batch

		-- Reproduce the tutorial's seven-bodies-per-frame spawn history with a
		-- deterministic fixed step. This preparation is never timed.
		if mode == "physics" then
			world.physics:StepSimulation(fixed_step, fixed_step, 1)
		elseif mode == "scene" or mode == "render" then
			hg.SceneUpdateSystems(world.scene, world.clocks, fixed_step, world.physics, fixed_step, 1)
		else
			hg.SceneUpdateSystems(world.scene, world.clocks, fixed_step)
		end
	end

	return world
end

local function step_world(world, submit_render)
	if mode == "physics" then
		world.physics:StepSimulation(fixed_step, fixed_step, 1)
	elseif mode == "scene" or mode == "render" then
		hg.SceneUpdateSystems(world.scene, world.clocks, fixed_step, world.physics, fixed_step, 1)
	else
		hg.SceneUpdateSystems(world.scene, world.clocks, fixed_step)
	end
	if render_context ~= nil and submit_render ~= false then
		hg.SubmitSceneToPipeline(0, world.scene, hg.IntRect(0, 0, render_context.width, render_context.height), true,
			render_context.pipeline, render_context.resources)
		hg.Frame()
		hg.UpdateWindow(render_context.window)
	end
end

local function run_steps(world, count, submit_render)
	for _ = 1, count do
		step_world(world, submit_render)
	end
end

local function count_sleeping_dynamic_nodes(world)
	if world.physics == nil or #world.nodes == 0 then
		return 0
	end
	local sleeping_count = 0
	for _, node in ipairs(world.nodes) do
		if world.physics:NodeIsSleeping(node) then
			sleeping_count = sleeping_count + 1
		end
	end
	return sleeping_count
end

local function wait_for_all_sleeping(world)
	local waited_steps = 0
	local sleeping_count = count_sleeping_dynamic_nodes(world)
	while sleeping_count ~= #world.nodes and waited_steps < sleep_max_steps do
		local step_count = math.min(sleep_poll_steps, sleep_max_steps - waited_steps)
		run_steps(world, step_count, false)
		waited_steps = waited_steps + step_count
		sleeping_count = count_sleeping_dynamic_nodes(world)
	end
	if sleeping_count == #world.nodes then
		return waited_steps, sleeping_count
	end
	return nil, sleeping_count
end

local function describe_first_awake_node(world)
	for index, node in ipairs(world.nodes) do
		if not world.physics:NodeIsSleeping(node) then
			local linear_velocity = world.physics:NodeGetLinearVelocity(node)
			local angular_velocity = world.physics:NodeGetAngularVelocity(node)
			return string.format("first awake node=%d linear=(%.6f, %.6f, %.6f) angular=(%.6f, %.6f, %.6f)",
				index, linear_velocity.x, linear_velocity.y, linear_velocity.z,
				angular_velocity.x, angular_velocity.y, angular_velocity.z)
		end
	end
	return "no awake node found"
end

local function percentile(sorted_samples, ratio)
	local index = math.floor((#sorted_samples - 1) * ratio) + 1
	return sorted_samples[index]
end

local function measure_phase(world, phase, repetition, seed, sleep_wait_steps)
	run_steps(world, warmup_steps)
	hg.EndProfilerFrame()

	local samples = {}
	local total_ns = 0
	for i = 1, sample_steps do
		local start = hg.time_now()
		step_world(world)
		local elapsed = hg.time_now() - start
		samples[i] = elapsed
		total_ns = total_ns + elapsed
	end

	if profiling then
		local profile = hg.CaptureProfilerFrame()
		print(string.format("[benchmark] Tau profile phase=%s repetition=%d", phase, repetition))
		hg.PrintProfilerFrame(profile)
	end
	hg.EndProfilerFrame()

	table.sort(samples)
	local median_ns = percentile(samples, 0.50)
	local p95_ns = percentile(samples, 0.95)
	local phase_semantics = phase == "active" and "active_drop" or
		(phase == "settled" and "fixed_cooldown" or "all_dynamic_bodies_sleeping")
	local sleep_wait_json = sleep_wait_steps ~= nil and tostring(sleep_wait_steps) or "null"
	local record = string.format(
		'{"schema":1,"timestamp_utc":%s,"backend":%s,"mode":%s,"phase":%s,"shape_mix":%s,"layout":%s,' ..
		'"bodies":%d,"static_bodies":5,"batch":%d,"seed":%d,"repetition":%d,"samples":%d,' ..
		'"fixed_step_ns":%d,"warmup_steps":%d,"settle_steps":%d,"phase_semantics":%s,' ..
		'"sleep_wait_steps":%s,"sleep_poll_steps":%d,"sleep_max_steps":%d,"profiled":%s,"diagnostics":%s,' ..
		'"total_ns":%d,"mean_us":%.3f,"median_us":%.3f,"p95_us":%.3f,"min_us":%.3f,"max_us":%.3f,' ..
		'"revision":%s,"build_config":%s,"computer":%s,"processor":%s,"logical_processors":%s}',
		json_string(os.date("!%Y-%m-%dT%H:%M:%SZ")), json_string(backend), json_string(mode), json_string(phase), json_string(shape_mix), json_string(layout),
		target_count, batch_size, seed, repetition, sample_steps, fixed_step, warmup_steps, settle_steps, json_string(phase_semantics),
		sleep_wait_json, sleep_poll_steps, sleep_max_steps, profiling and "true" or "false", diagnostics_enabled and "true" or "false",
		total_ns, hg.time_to_us_f(total_ns) / sample_steps, hg.time_to_us_f(median_ns), hg.time_to_us_f(p95_ns),
		hg.time_to_us_f(samples[1]), hg.time_to_us_f(samples[#samples]), json_string(env_string("BENCH_REVISION", "unknown")),
		json_string(env_string("BENCH_BUILD_CONFIG", "unknown")), json_string(env_string("COMPUTERNAME", "unknown")),
		json_string(env_string("PROCESSOR_IDENTIFIER", "unknown")), json_string(env_string("NUMBER_OF_PROCESSORS", "unknown")))

	output:write(record, "\n")
	print(string.format("[benchmark] backend=%s mode=%s phase=%s shapes=%s layout=%s bodies=%d repetition=%d median=%.3f us p95=%.3f us",
		backend, mode, phase, shape_mix, layout, target_count, repetition, hg.time_to_us_f(median_ns), hg.time_to_us_f(p95_ns)))
end

print(string.format("[benchmark] backend=%s mode=%s phase=%s shapes=%s layout=%s bodies=%d samples=%d repetitions=%d output=%s",
	backend, mode, phase_selection, shape_mix, layout, target_count, sample_steps, repetitions, output_path))

for repetition = 1, repetitions do
	local seed = base_seed + repetition - 1
	local world = make_world(seed)

	if phase_selection == "active" or phase_selection == "both" then
		measure_phase(world, "active", repetition, seed)
	end

	if phase_selection == "settled" or phase_selection == "both" then
		-- Rendering does not affect the deterministic physics state. Skip GPU
		-- submission during this untimed transition and warm the complete render
		-- path again before measuring the settled window.
		run_steps(world, settle_steps, false)
		measure_phase(world, "settled", repetition, seed)
	end

	if phase_selection == "all_sleeping" then
		local sleep_wait_steps, sleeping_count = wait_for_all_sleeping(world)
		assert(sleep_wait_steps ~= nil, string.format(
			"only %d/%d dynamic bodies slept within BENCH_SLEEP_MAX=%d steps (backend=%s repetition=%d seed=%d; %s)",
			sleeping_count, #world.nodes, sleep_max_steps, backend, repetition, seed, describe_first_awake_node(world)))
		print(string.format("[benchmark] backend=%s repetition=%d all dynamic bodies sleeping after %d wait steps",
			backend, repetition, sleep_wait_steps))
		measure_phase(world, "all_sleeping", repetition, seed, sleep_wait_steps)
	end

	if world.physics ~= nil then
		world.physics:Clear()
	end
	world = nil
	collectgarbage("collect")
end

output:close()
if render_context ~= nil then
	hg.RenderShutdown()
	hg.DestroyWindow(render_context.window)
	hg.WindowSystemShutdown()
	hg.InputShutdown()
end
print("[benchmark] done")

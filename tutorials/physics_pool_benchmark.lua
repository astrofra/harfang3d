-- Deterministic, headless benchmark for the physics_pool_of_objects workload.
--
-- Run this script with the matching Bullet or Tau HG Lua package. Example:
--
--   set BENCH_BACKEND=tau
--   set BENCH_BODIES=1500
--   set BENCH_OUTPUT=physics_pool_tau.jsonl
--   ..\..\install\tau\hg_lua\lua.exe physics_pool_benchmark.lua
--
-- Set HG_TAU_PROFILE=1 for aggregate Tau phase timings. Profiled results are
-- marked in JSON and should be used for attribution, not backend comparison.

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
assert(mode == "physics" or mode == "scene" or mode == "no_physics",
	"BENCH_MODE must be 'physics', 'scene', or 'no_physics'")

local phase_selection = env_string("BENCH_PHASE", "both")
assert(phase_selection == "active" or phase_selection == "settled" or phase_selection == "both",
	"BENCH_PHASE must be 'active', 'settled', or 'both'")

local shape_mix = env_string("BENCH_SHAPES", "mixed")
assert(shape_mix == "mixed" or shape_mix == "cube" or shape_mix == "sphere",
	"BENCH_SHAPES must be 'mixed', 'cube', or 'sphere'")

local target_count = math.floor(env_number("BENCH_BODIES", 1500))
local batch_size = math.floor(env_number("BENCH_BATCH", 7))
local sample_steps = math.floor(env_number("BENCH_SAMPLES", 120))
local warmup_steps = math.floor(env_number("BENCH_WARMUP", 10))
local settle_steps = math.floor(env_number("BENCH_SETTLE", 600))
local repetitions = math.floor(env_number("BENCH_REPETITIONS", 5))
local base_seed = math.floor(env_number("BENCH_SEED", 0x544155))
local fixed_step = hg.time_from_sec_f(1 / 60)
local output_path = env_string("BENCH_OUTPUT", string.format("physics_pool_%s.jsonl", backend))
local append_output = env_flag("BENCH_APPEND")
local profiling = env_flag("HG_TAU_PROFILE")

assert(target_count > 0, "BENCH_BODIES must be positive")
assert(batch_size > 0, "BENCH_BATCH must be positive")
assert(sample_steps > 0, "BENCH_SAMPLES must be positive")
assert(warmup_steps >= 0, "BENCH_WARMUP must not be negative")
assert(settle_steps >= 0, "BENCH_SETTLE must not be negative")
assert(repetitions > 0, "BENCH_REPETITIONS must be positive")

if profiling and backend ~= "tau" then
	print("[benchmark] warning: HG_TAU_PROFILE is enabled for a Bullet run")
end
if profiling and env_flag("HG_TAU_CONTACT_DIAGNOSTICS") then
	print("[benchmark] warning: contact diagnostics logging will perturb profiled timings")
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

local function create_collision_body(scene, collision_type, position, size, radius, body_type, mass)
	local node = scene:CreateNode()
	node:SetTransform(scene:CreateTransform(position))

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

local function create_container(scene)
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(0, -0.5, 0), hg.Vec3(30, 1, 30), 0, hg.RBT_Static, 0)
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(-15.5, -0.5, 0), hg.Vec3(1, 11, 32), 0, hg.RBT_Static, 0)
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(15.5, -0.5, 0), hg.Vec3(1, 11, 32), 0, hg.RBT_Static, 0)
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(0, -0.5, -15.5), hg.Vec3(32, 11, 1), 0, hg.RBT_Static, 0)
	create_collision_body(scene, hg.CT_Cube, hg.Vec3(0, -0.5, 15.5), hg.Vec3(32, 11, 1), 0, hg.RBT_Static, 0)
end

local function create_dynamic_body(scene, rng)
	local collision_type
	if shape_mix == "cube" then
		collision_type = hg.CT_Cube
	elseif shape_mix == "sphere" then
		collision_type = hg.CT_Sphere
	else
		collision_type = rng() > 0.5 and hg.CT_Cube or hg.CT_Sphere
	end

	local position = hg.Vec3(-10 + rng() * 20, 18, -10 + rng() * 20)
	return create_collision_body(scene, collision_type, position, hg.Vec3(1, 1, 1), 0.5, hg.RBT_Dynamic, 1)
end

local function make_world(seed)
	local world = {
		scene = hg.Scene(),
		clocks = hg.SceneClocks(),
		physics = nil,
		nodes = {},
	}
	create_container(world.scene)

	if mode ~= "no_physics" then
		world.physics = hg.ScenePhysics()
		world.physics:SceneCreatePhysicsFromAssets(world.scene)
	end

	local rng = make_rng(seed)
	local created = 0
	while created < target_count do
		local count_this_batch = math.min(batch_size, target_count - created)
		for _ = 1, count_this_batch do
			local node = create_dynamic_body(world.scene, rng)
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
		elseif mode == "scene" then
			hg.SceneUpdateSystems(world.scene, world.clocks, fixed_step, world.physics, fixed_step, 1)
		else
			hg.SceneUpdateSystems(world.scene, world.clocks, fixed_step)
		end
	end

	return world
end

local function step_world(world)
	if mode == "physics" then
		world.physics:StepSimulation(fixed_step, fixed_step, 1)
	elseif mode == "scene" then
		hg.SceneUpdateSystems(world.scene, world.clocks, fixed_step, world.physics, fixed_step, 1)
	else
		hg.SceneUpdateSystems(world.scene, world.clocks, fixed_step)
	end
end

local function run_steps(world, count)
	for _ = 1, count do
		step_world(world)
	end
end

local function percentile(sorted_samples, ratio)
	local index = math.floor((#sorted_samples - 1) * ratio) + 1
	return sorted_samples[index]
end

local function measure_phase(world, phase, repetition, seed)
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
	local record = string.format(
		'{"schema":1,"timestamp_utc":%s,"backend":%s,"mode":%s,"phase":%s,"shape_mix":%s,' ..
		'"bodies":%d,"static_bodies":5,"batch":%d,"seed":%d,"repetition":%d,"samples":%d,' ..
		'"fixed_step_ns":%d,"warmup_steps":%d,"settle_steps":%d,"profiled":%s,' ..
		'"total_ns":%d,"mean_us":%.3f,"median_us":%.3f,"p95_us":%.3f,"min_us":%.3f,"max_us":%.3f,' ..
		'"revision":%s,"build_config":%s,"computer":%s,"processor":%s,"logical_processors":%s}',
		json_string(os.date("!%Y-%m-%dT%H:%M:%SZ")), json_string(backend), json_string(mode), json_string(phase), json_string(shape_mix),
		target_count, batch_size, seed, repetition, sample_steps, fixed_step, warmup_steps, settle_steps, profiling and "true" or "false",
		total_ns, hg.time_to_us_f(total_ns) / sample_steps, hg.time_to_us_f(median_ns), hg.time_to_us_f(p95_ns),
		hg.time_to_us_f(samples[1]), hg.time_to_us_f(samples[#samples]), json_string(env_string("BENCH_REVISION", "unknown")),
		json_string(env_string("BENCH_BUILD_CONFIG", "unknown")), json_string(env_string("COMPUTERNAME", "unknown")),
		json_string(env_string("PROCESSOR_IDENTIFIER", "unknown")), json_string(env_string("NUMBER_OF_PROCESSORS", "unknown")))

	output:write(record, "\n")
	print(string.format("[benchmark] backend=%s mode=%s phase=%s shapes=%s bodies=%d repetition=%d median=%.3f us p95=%.3f us",
		backend, mode, phase, shape_mix, target_count, repetition, hg.time_to_us_f(median_ns), hg.time_to_us_f(p95_ns)))
end

print(string.format("[benchmark] backend=%s mode=%s phase=%s shapes=%s bodies=%d samples=%d repetitions=%d output=%s",
	backend, mode, phase_selection, shape_mix, target_count, sample_steps, repetitions, output_path))

for repetition = 1, repetitions do
	local seed = base_seed + repetition - 1
	local world = make_world(seed)

	if phase_selection == "active" or phase_selection == "both" then
		measure_phase(world, "active", repetition, seed)
	end

	if phase_selection == "settled" or phase_selection == "both" then
		run_steps(world, settle_steps)
		measure_phase(world, "settled", repetition, seed)
	end

	if world.physics ~= nil then
		world.physics:Clear()
	end
	world = nil
	collectgarbage("collect")
end

output:close()
print("[benchmark] done")

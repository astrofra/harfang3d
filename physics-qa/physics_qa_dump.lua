local M = {}

local function GetEnvNumber(name, default)
	local value = tonumber(os.getenv(name))
	return value and value > 0 and value or default
end

local function VectorToJson(v)
	return string.format("[%.9g,%.9g,%.9g]", v.x, v.y, v.z)
end

local function MatrixToJson(m)
	local x, y, z, t = hg.GetX(m), hg.GetY(m), hg.GetZ(m), hg.GetT(m)
	return string.format("[%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g]",
		x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z, t.x, t.y, t.z)
end

local function EscapeJson(value)
	return value:gsub('\\', '\\\\'):gsub('"', '\\"')
end

function M.Create(test_name, physics_step, nodes)
	local enabled = os.getenv("HG_PHYSICS_QA_MODE") == "dump_matrix"
	local dump = {enabled = enabled, complete = false}
	if not enabled then
		return dump
	end

	local backend = os.getenv("HG_PHYSICS_QA_BACKEND") or "unknown"
	local output_dir = "qa_dumps/" .. backend
	os.execute('if not exist "' .. output_dir .. '" mkdir "' .. output_dir .. '"')

	dump.sample_interval = GetEnvNumber("HG_PHYSICS_QA_DUMP_EVERY", 1)
	dump.max_samples = GetEnvNumber("HG_PHYSICS_QA_DUMP_SAMPLES", 600)
	dump.physics_step = hg.time_to_sec_f(physics_step)
	dump.nodes = nodes
	dump.sample_count = 0
	dump.step_count = 0
	dump.path = output_dir .. "/" .. test_name .. ".jsonl"
	dump.test_name = test_name
	dump.backend = backend
	dump.records = {}
	print("Physics QA dump: " .. dump.path)
	return dump
end

function M.Capture(dump, physics)
	if not dump.enabled or dump.complete then
		return dump.complete
	end

	dump.step_count = dump.step_count + 1
	if dump.step_count % dump.sample_interval ~= 0 then
		return false
	end

	local record = {t = dump.step_count * dump.physics_step, frame_nodes = {}, linear_velocities = {}, angular_velocities = {}}
	for index, node in ipairs(dump.nodes) do
		record.frame_nodes[index] = node:GetTransform():GetWorld()
		record.linear_velocities[index] = physics:NodeGetLinearVelocity(node)
		record.angular_velocities[index] = physics:NodeGetAngularVelocity(node)
	end
	table.insert(dump.records, record)
	dump.sample_count = dump.sample_count + 1
	if dump.sample_count >= dump.max_samples then
		dump.complete = true
		print(string.format("Physics QA dump complete: %d samples", dump.sample_count))
	end
	return dump.complete
end

function M.Close(dump)
	if dump.enabled and dump.records then
		local file = assert(io.open(dump.path, "w"))
		file:write(string.format('{"type":"metadata","test":"%s","backend":"%s","physics_step":%.9g,"sample_interval":%d,"sample_count":%d}\n',
			EscapeJson(dump.test_name), EscapeJson(dump.backend), dump.physics_step, dump.sample_interval, #dump.records))
		for sample_index, record in ipairs(dump.records) do
			local bodies = {}
			for index, world in ipairs(record.frame_nodes) do
				bodies[#bodies + 1] = string.format('{"index":%d,"world":%s,"linear_velocity":%s,"angular_velocity":%s}',
					index, MatrixToJson(world), VectorToJson(record.linear_velocities[index]), VectorToJson(record.angular_velocities[index]))
			end
			file:write(string.format('{"type":"sample","step":%d,"time":%.9g,"bodies":[%s]}\n',
				sample_index * dump.sample_interval, record.t, table.concat(bodies, ",")))
		end
		file:close()
		dump.records = nil
	end
end

return M

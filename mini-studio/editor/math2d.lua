local M = {}

function M.clamp(value, min_value, max_value)
	if value < min_value then
		return min_value
	end
	if value > max_value then
		return max_value
	end
	return value
end

function M.length(x, y)
	return math.sqrt(x * x + y * y)
end

function M.distance(a, b)
	return M.length(a.x - b.x, a.y - b.y)
end

function M.normalize(x, y)
	local len = M.length(x, y)
	if len <= 0.000001 then
		return 0, 0, 0
	end
	return x / len, y / len, len
end

function M.distance_to_segment(p, a, b)
	local abx = b.x - a.x
	local aby = b.y - a.y
	local apx = p.x - a.x
	local apy = p.y - a.y
	local ab_len2 = abx * abx + aby * aby

	if ab_len2 <= 0.000001 then
		return M.distance(p, a), a
	end

	local t = M.clamp((apx * abx + apy * aby) / ab_len2, 0, 1)
	local closest = { x = a.x + abx * t, y = a.y + aby * t }
	return M.distance(p, closest), closest
end

function M.angle_from(center, p)
	return math.atan(p.y - center.y, p.x - center.x)
end

return M

local hg = require("harfang")

local function fail(message)
	error("launcher regression: " .. message, 0)
end

if LAUNCHER_ASSETS_SOURCE ~= "folder" and LAUNCHER_ASSETS_SOURCE ~= "legacy" then
	fail("unexpected assets source " .. tostring(LAUNCHER_ASSETS_SOURCE))
end

if LAUNCHER_ASSETS_SOURCE == "folder" then
	if LAUNCHER_DATA_DIR == nil or LAUNCHER_DATA_DIR == "" then
		fail("expected LAUNCHER_DATA_DIR in folder mode")
	end
	if LAUNCHER_DATA_PACKAGE ~= nil and LAUNCHER_DATA_PACKAGE ~= "" then
		fail("unexpected LAUNCHER_DATA_PACKAGE in folder mode")
	end
else
	if LAUNCHER_DATA_PACKAGE == nil or LAUNCHER_DATA_PACKAGE == "" then
		fail("expected LAUNCHER_DATA_PACKAGE in archive mode")
	end
end

if not hg.AddAssetsFolder("data/assets") then
	fail("hg.AddAssetsFolder('data/assets') failed")
end

local js = hg.LoadJsonFromAssets("messages/hello.json")
local ok, hello = hg.GetJsonString(js, "message")
if not ok then
	fail("GetJsonString failed")
end
if hello ~= "hello launcher" then
	fail("unexpected asset content " .. tostring(hello))
end

local smoke = require("pkg.smoke")
if smoke.probe ~= "lua-nested" then
	fail("unexpected module probe " .. tostring(smoke.probe))
end

print("launcher-regression-ok language=lua source=" .. LAUNCHER_ASSETS_SOURCE)

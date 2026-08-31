local hg = require("harfang");

function fail(message) {
	throw "launcher regression: " + message;
}

if (LAUNCHER_ASSETS_SOURCE != "folder" && LAUNCHER_ASSETS_SOURCE != "legacy") {
	fail("unexpected assets source " + LAUNCHER_ASSETS_SOURCE);
}

if (LAUNCHER_ASSETS_SOURCE == "folder") {
	if (LAUNCHER_DATA_DIR == null || LAUNCHER_DATA_DIR == "") {
		fail("expected LAUNCHER_DATA_DIR in folder mode");
	}
	if (LAUNCHER_DATA_PACKAGE != "") {
		fail("unexpected LAUNCHER_DATA_PACKAGE in folder mode");
	}
} else {
	if (LAUNCHER_DATA_PACKAGE == null || LAUNCHER_DATA_PACKAGE == "") {
		fail("expected LAUNCHER_DATA_PACKAGE in archive mode");
	}
}

if (!hg.AddAssetsFolder("data/assets")) {
	fail("hg.AddAssetsFolder('data/assets') failed");
}

local js = hg.LoadJsonFromAssets("messages/hello.json");
local get_result = hg.GetJsonString(js, "message");
if (get_result.len() != 2 || !get_result[0]) {
	fail("GetJsonString failed");
}
local hello = get_result[1];
if (hello != "hello launcher") {
	fail("unexpected asset content " + hello);
}

::launcher_regression_probe <- null;
include("pkg/smoke.nut");
if (::launcher_regression_probe != "squirrel-nested") {
	fail("unexpected include probe " + ::launcher_regression_probe);
}

print("launcher-regression-ok language=squirrel source=" + LAUNCHER_ASSETS_SOURCE + "\n");

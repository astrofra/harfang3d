// Creating a Lua script VM from Squirrel.
// Current Squirrel bindings expose SceneLuaVM and LuaObject wrappers, but not
// high-level LuaObject Pack/Unpack helpers like the Python host binding.
// This tutorial therefore validates scene Lua VM creation and host->guest calls
// that do not require typed value marshalling yet.

local hg = require("harfang");

local scene = hg.Scene();
local script = scene.CreateScript("example");

local lua_vm = hg.SceneLuaVM();
local src =
	"a = 4\n" +
	"\n" +
	"function CallToPrintA() print('CallToPrintA: '..a) end\n" +
	"function SetA24() a = 24; print('SetA24: '..a) end\n";

assert(lua_vm.CreateScriptFromSource(scene, script, src) == true);

local call_a = lua_vm.Call(script, "CallToPrintA", []);
assert(call_a[0] == true);

local call_set = lua_vm.Call(script, "SetA24", []);
assert(call_set[0] == true);

local invalid_call = lua_vm.Call(script, "InvalidCall", []);
assert(invalid_call[0] == false);

print("scene_lua_script.nut: SceneLuaVM host->guest calls ok\n");
print("scene_lua_script.nut: LuaObject Pack/Unpack is not exposed in the current Squirrel binding, so typed Get/Set/arg/return transfer remains TODO\n");

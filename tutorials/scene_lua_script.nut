// Creating a Lua script VM from Squirrel and exchanging typed values with it.

local hg = require("harfang");

local scene = hg.Scene();
local script = scene.CreateScript("example");

local lua_vm = hg.SceneLuaVM();
local src =
	"a = 4\n" +
	"\n" +
	"function CallToPrintA() print('CallToPrintA: '..a) end\n" +
	"function CallToPrintV(v) print('CallToPrintV: '..v) end\n" +
	"function CallToPrintScriptPath(s) print('CallToPrintScriptPath: '..s:GetPath()) end\n" +
	"function SetA24() a = 24; print('SetA24: '..a) end\n" +
	"function CallToReturnValue() return 'String returned from scene VM to host VM' end\n";

assert(lua_vm.CreateScriptFromSource(scene, script, src) == true);

local a = lua_vm.GetScriptValue(script, "a");
assert(lua_vm.Unpack(a) == 4);
print("GetScriptValue returned a=" + lua_vm.Unpack(a).tostring() + "\n");

assert(lua_vm.SetScriptValue(script, "a", lua_vm.Pack(24)) == true);

local updated_a = lua_vm.GetScriptValue(script, "a");
assert(lua_vm.Unpack(updated_a) == 24);
print("GetScriptValue returned a=" + lua_vm.Unpack(updated_a).tostring() + "\n");

local call_a = lua_vm.Call(script, "CallToPrintA", []);
assert(call_a[0] == true);

local call_v = lua_vm.Call(script, "CallToPrintV", [lua_vm.Pack(8)]);
assert(call_v[0] == true);

local call_script = lua_vm.Call(script, "CallToPrintScriptPath", [lua_vm.Pack(script)]);
assert(call_script[0] == true);

local call_set = lua_vm.Call(script, "SetA24", []);
assert(call_set[0] == true);

local invalid_call = lua_vm.Call(script, "InvalidCall", []);
assert(invalid_call[0] == false);

local return_call = lua_vm.Call(script, "CallToReturnValue", []);
assert(return_call[0] == true);
assert(lua_vm.Unpack(return_call[1][0]) == "String returned from scene VM to host VM");
print("CallToReturnValue return value=" + lua_vm.Unpack(return_call[1][0]).tostring() + "\n");

print("scene_lua_script.nut: SceneLuaVM host<->guest Pack/Unpack typed transfer ok\n");

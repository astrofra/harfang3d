// Creating a Lua script VM for a scene object and communicating with a Script component

import * as hg from 'harfang';

export function main() {
	const scene = new hg.Scene();
	const script = scene.CreateScript('example');

	const lua_vm = new hg.SceneLuaVM();
	try {
		if (!lua_vm.CreateScriptFromSource(scene, script, `
a = 4

function CallToPrintA() print('CallToPrintA: '..a) end
function CallToPrintV(v) print('CallToPrint: '..v) end
function CallToPrintScriptPath(s) print('CallToPrintScriptPath: '..s:GetPath()) end

function CallToReturnValue() return 'String returned from scene VM to host VM' end
`)) {
			throw Error('Failed to create Lua script');
		}

		// Pack/Unpack transfer typed values between the JavaScript and Lua VMs.
		let a = lua_vm.Unpack(lua_vm.GetScriptValue(script, 'a'));
		console.log('GetScriptValue returned a=' + a);
		if (a !== 4n) throw Error('Unexpected initial value'); // Lua integers return as BigInt

		if (!lua_vm.SetScriptValue(script, 'a', lua_vm.Pack(24n))) {
			throw Error('Failed to set script value');
		}

		a = lua_vm.Unpack(lua_vm.GetScriptValue(script, 'a'));
		console.log('GetScriptValue returned a=' + a);
		if (a !== 24n) throw Error('Unexpected updated value');

		if (!lua_vm.Call(script, 'CallToPrintA', [])[0]) throw Error('CallToPrintA failed');
		if (!lua_vm.Call(script, 'CallToPrintV', [lua_vm.Pack(8)])[0]) throw Error('CallToPrintV failed');
		if (!lua_vm.Call(script, 'CallToPrintScriptPath', [lua_vm.Pack(script)])[0]) throw Error('CallToPrintScriptPath failed');

		if (lua_vm.Call(script, 'InvalidCall', [])[0]) throw Error('InvalidCall unexpectedly succeeded');

		const [success, rvalues] = lua_vm.Call(script, 'CallToReturnValue', []);
		if (!success) throw Error('CallToReturnValue failed');

		// Native result lists use zero-based get()/at() access.
		const value = lua_vm.Unpack(rvalues.at(0));
		if (value !== 'String returned from scene VM to host VM') throw Error('Unexpected return value');
		console.log('CallToReturnValue return value=' + value);
	} finally {
		hg.SceneClearSystems(scene, lua_vm);
	}
}

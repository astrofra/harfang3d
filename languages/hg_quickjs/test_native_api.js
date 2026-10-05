import * as hg from 'harfang';

function assert(value, message) { if (!value) throw Error(message); }

// External languages must be able to drive and communicate with Lua components.
for (let round = 0; round < 8; ++round) {
    const scene = new hg.Scene(), vm = new hg.SceneLuaVM();
    const script = scene.CreateScript('inline.lua');
    assert(vm.CreateScriptFromSource(scene, script, `
        value = 0
        function echo(v) return v, type(v) end
        function translated(v) return v + hg.Vec3(1, 2, 3) end
    `), 'CreateScriptFromSource');
    const cases = [null, false, true, -9223372036854775808n, 9223372036854775807n,
        9007199254740993n, 2.25, '123.5', 'a\0b', 'café'];
    for (const value of cases) {
        const packed = vm.Pack(value);
        assert(vm.Unpack(packed) === value, 'Pack/Unpack roundtrip: ' + value);
        assert(vm.SetScriptValue(script, 'value', packed), 'SetScriptValue');
        assert(vm.Unpack(vm.GetScriptValue(script, 'value')) === value, 'GetScriptValue');
        const [ok, results] = vm.Call(script, 'echo', [packed]);
        assert(ok && results.size() === 2n, 'Call output values');
        assert(vm.Unpack(results.at(0)) === value, 'Call return value');
    }
    assert(vm.Unpack(vm.Pack(42)) === 42n, 'JS integral Number to Lua integer');
    assert(vm.Unpack(vm.Pack(undefined)) === null, 'Undefined to Lua nil');
    const vector = new hg.Vec3(4, 5, 6), packed = vm.Pack(vector);
    vector.x = 99;
    assert(vm.Unpack(packed).x === 4, 'Pack copies native values');
    const unpacked = vm.Unpack(packed);
    unpacked.y = 99;
    assert(vm.Unpack(packed).y === 5, 'Unpack copies native values');
    const [ok, results] = vm.Call(script, 'translated', [packed]);
    const translated = vm.Unpack(results.at(0));
    assert(ok && translated.x === 5 && translated.y === 7 && translated.z === 9, 'Native object exchange');
    for (const invalid of [1n << 63n, -(1n << 63n) - 1n, {}, () => {}]) {
        let rejected = false;
        try { vm.Pack(invalid); } catch (_) { rejected = true; }
        assert(rejected, 'Unsupported/out-of-range Lua conversion');
    }
    hg.SceneClearSystems(scene, vm);
}

// Results keep the source VM alive even if JS releases its own reference to it.
let packed = new hg.SceneLuaVM().Pack(9007199254740993n);
const reader = new hg.SceneLuaVM();
assert(reader.Unpack(packed) === 9007199254740993n, 'Temporary VM lifetime');
packed = null;
function temporaryResults() {
    const scene = new hg.Scene(), vm = new hg.SceneLuaVM(), script = scene.CreateScript('temporary.lua');
    vm.CreateScriptFromSource(scene, script, 'function echo() return 123, hg.Vec3(7, 8, 9) end');
    return vm.Call(script, 'echo', [])[1];
}
let results = temporaryResults();
let item = results.at(1);
assert(reader.Unpack(results.at(0)) === 123n, 'Return list keeps VM alive');
results = null;
assert(reader.Unpack(item).z === 9, 'List element keeps VM alive');
item = null;
const source = [new hg.SceneLuaVM().Pack(17n)];
let list = new hg.LuaObjectList(source);
source.length = 0;
assert(reader.Unpack(list.at(0)) === 17n, 'List constructor retains references');
list.push_back(new hg.SceneLuaVM().Pack(23n));
assert(reader.Unpack(list.get(1)) === 23n, 'List insertion retains references');
list.set(0, new hg.SceneLuaVM().Pack(42n));
assert(reader.Unpack(list.get(0)) === 42n, 'List replacement retains references');
list.clear();
list = null;

// The full Bullet profile uses the same native physics systems as Lua/Squirrel.
if (hg.GetScenePhysicsBackendName() === 'Bullet Physics') {
    const scene = new hg.Scene(), physics = new hg.ScenePhysics(), vm = new hg.SceneLuaVM();
    try {
        const ball = scene.CreateNode('ball');
        ball.SetTransform(scene.CreateTransform(new hg.Vec3(0, 2, 0)));
        const body = scene.CreateRigidBody(); body.SetType(hg.RBT_Dynamic); ball.SetRigidBody(body);
        const shape = scene.CreateCollision(); shape.SetType(hg.CT_Sphere); shape.SetRadius(.5); shape.SetMass(1);
        ball.SetCollision(0, shape);
        scene.Update(0n);
        hg.SceneSyncToSystemsFromAssets(scene, physics, vm);
        assert(physics.NodeHasBody(ball), 'Bullet body');
        const clocks = new hg.SceneClocks();
        for (let i = 0; i < 60; ++i) hg.SceneUpdateSystems(scene, clocks, 16666667n, physics, 16666667n, 8, vm);
        assert(hg.GetT(ball.GetTransform().GetWorld()).y < 1, 'Bullet gravity');
    } finally { hg.SceneClearSystems(scene, physics, vm); }
    console.log('HGJS_BULLET_OK');
}
console.log('HGJS_NATIVE_API_OK');

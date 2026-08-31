// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene_systems.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"

#include "engine/assets_rw_interface.h"
#include "engine/scene_lua_vm.h"
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
#include "engine/scene_bullet3_physics.h"
#endif
#if HG_ENABLE_TAU_SCENE_PHYSICS
#include "engine/scene_tau_physics.h"
#endif
#if HG_ENABLE_BULLET3_SCENE_PHYSICS || HG_ENABLE_TAU_SCENE_PHYSICS
#include "engine/scene_physics.h"
#endif
#include "engine/physics.h"
#include "engine/scene.h"

#include "fabgen.h"

#include "bind_Lua.h"

namespace hg {

//
static void SceneSyncToScriptsCall(SceneLuaVM &vm, Scene &scene, const Reader &ir, const ReadProvider &ip) {
	const auto scripts = vm.SceneCreateScripts(scene, ir, ip);

	vm.ForeachScriptsInSceneContext(scene, scripts, [&](Scene &scene, const LuaObject &env) {
		if (auto on_create = Get(env, "OnAttachToScene")) {
			on_create.Push();
			hg_lua_OnAttachToScene(env.L(), -1, scene);
		}
	});

	vm.ForeachScriptsInNodeContext(scene, scripts, [&](const Scene &scene, Node &node, const LuaObject &env) {
		if (auto on_create = Get(env, "OnAttachToNode")) {
			on_create.Push();
			hg_lua_OnAttachToNode(env.L(), -1, node);
		}
	});
}

//
static void SceneScriptsOnUpdateCall(SceneLuaVM &vm, Scene &scene, time_ns dt) {
	vm.ForeachSceneScripts(scene, [&](Scene &scene, const LuaObject &env) {
		if (auto on_create = Get(env, "OnUpdate")) {
			on_create.Push();
			hg_lua_OnUpdate_SceneCtx(env.L(), -1, scene, dt);
		}
	});

	vm.ForeachAllNodesScripts(scene, [&](const Scene &scene, Node &node, const LuaObject &env) {
		if (auto on_create = Get(env, "OnUpdate")) {
			on_create.Push();
			hg_lua_OnUpdate_NodeCtx(env.L(), -1, node, dt);
		}
	});
}

static void SceneScriptsOnCollisionCall(SceneLuaVM &vm, Scene &scene, const NodePairContacts &node_node_contacts) {
	for (auto i : node_node_contacts)
		vm.ForeachNodeScripts(scene, i.first, [&](const Scene &scene, Node &node, const LuaObject &env) {
			if (auto on_create = Get(env, "OnCollision")) {
				for (auto j : i.second) {
					auto with = scene.GetNode(j.first);
					on_create.Push();
					hg_lua_OnCollision(env.L(), -1, node, with, j.second);
				}
			}
		});
}

//
static void CallScriptAttachToScene(SceneLuaVM &vm, Scene &scene, ComponentRef ref) {
	auto env = vm.GetScriptEnv(ref);

	if (auto fn = Get(env, "OnAttachToScene")) {
		fn.Push();
		hg_lua_OnAttachToScene(env.L(), -1, scene);
	}
}

static void CallScriptAttachToNode(SceneLuaVM &vm, Node &node, ComponentRef ref) {
	auto env = vm.GetScriptEnv(ref);

	if (auto fn = Get(env, "OnAttachToNode")) {
		fn.Push();
		hg_lua_OnAttachToNode(env.L(), -1, node);
	}
}

static void CallScriptAttachToScene(SceneLuaVM &vm, Scene &scene, const Script &script) { CallScriptAttachToScene(vm, scene, script.ref); }
static void CallScriptAttachToNode(SceneLuaVM &vm, Node &node, const Script &script) { CallScriptAttachToNode(vm, node, script.ref); }

//
static void CallScriptDetachFromScene(SceneLuaVM &vm, Scene &scene, ComponentRef ref) {
	auto env = vm.GetScriptEnv(ref);

	if (auto fn = Get(env, "OnDetachFromScene")) {
		fn.Push();
		hg_lua_OnDetachFromScene(env.L(), -1, scene);
	}
}

static void CallScriptDetachFromNode(SceneLuaVM &vm, Node &node, ComponentRef ref) {
	auto env = vm.GetScriptEnv(ref);

	if (auto fn = Get(env, "OnDetachFromNode")) {
		fn.Push();
		hg_lua_OnDetachFromNode(env.L(), -1, node);
	}
}

static void CallScriptDetachFromScene(SceneLuaVM &vm, Scene &scene, const Script &script) { CallScriptDetachFromScene(vm, scene, script.ref); }
static void CallScriptDetachFromNode(SceneLuaVM &vm, Node &node, const Script &script) { CallScriptDetachFromNode(vm, node, script.ref); }

//
static size_t SceneScriptsDestroyGarbageCall(SceneLuaVM &vm, Scene &scene) {
	const auto garbage = vm.GarbageCollect(scene);
	vm.DestroyScripts(garbage);
	return garbage.size();
}

//
void SceneSyncToSystemsFromFile(Scene &scene, SceneLuaVM &vm) { SceneSyncToScriptsCall(vm, scene, g_file_reader, g_file_read_provider); }
void SceneSyncToSystemsFromAssets(Scene &scene, SceneLuaVM &vm) { SceneSyncToScriptsCall(vm, scene, g_assets_reader, g_assets_read_provider); }

template <typename PhysicsT>
static void SceneSyncToSystemsWithPhysics(Scene &scene, PhysicsT &physics, SceneLuaVM *vm, const Reader &ir, const ReadProvider &ip) {
	if (vm)
		SceneSyncToScriptsCall(*vm, scene, ir, ip);
	physics.SceneCreatePhysics(scene, ir, ip);
}

static void SceneUpdateSystemsNoPhysicsImpl(Scene &scene, SceneClocks &clocks, time_ns dt, SceneLuaVM *vm) {
	(void)clocks;

	scene.StorePreviousWorldMatrices();
	scene.ReadyWorldMatrices();

	scene.UpdatePlayingAnims(dt);

	if (vm)
		SceneScriptsOnUpdateCall(*vm, scene, dt); // calls OnUpdate in context

	scene.ComputeWorldMatrices();
	scene.FixupPreviousWorldMatrices();
}

template <typename PhysicsT>
static void SceneUpdateSystemsWithPhysicsImpl(Scene &scene, SceneClocks &clocks, time_ns dt, PhysicsT &physics, NodePairContacts *node_node_contacts,
	time_ns physics_step, int max_physics_step, SceneLuaVM *vm) {
	(void)clocks;

	scene.StorePreviousWorldMatrices();
	scene.ReadyWorldMatrices();

	scene.UpdatePlayingAnims(dt);

	physics.SyncTransformsFromScene(scene);
	physics.StepSimulation(dt, physics_step, max_physics_step);
	physics.SyncTransformsToScene(scene);

	if (node_node_contacts) {
		physics.CollectCollisionEvents(scene, *node_node_contacts);

		if (vm)
			SceneScriptsOnCollisionCall(*vm, scene, *node_node_contacts);
	}

	if (vm)
		SceneScriptsOnUpdateCall(*vm, scene, dt); // calls OnUpdate in context

	scene.ComputeWorldMatrices();
	scene.FixupPreviousWorldMatrices();
}

static size_t SceneGarbageCollectSystemsNoPhysicsImpl(Scene &scene, SceneLuaVM *vm) {
	size_t total = 0;

	total += scene.GarbageCollect();

	if (vm)
		total += SceneScriptsDestroyGarbageCall(*vm, scene); // calls OnDestroy in context

	return total;
}

template <typename PhysicsT>
static size_t SceneGarbageCollectSystemsWithPhysicsImpl(Scene &scene, PhysicsT &physics, SceneLuaVM *vm) {
	size_t total = 0;

	total += scene.GarbageCollect();
	total += physics.GarbageCollect(scene);
	total += physics.GarbageCollectResources();

	if (vm)
		total += SceneScriptsDestroyGarbageCall(*vm, scene); // calls OnDestroy in context

	return total;
}

static void SceneClearSystemsNoPhysicsImpl(Scene &scene, SceneLuaVM *vm) {
	if (vm) {
		vm->ForeachScripts([](const LuaObject &env) {
			if (auto on_destroy = Get(env, "OnDestroy")) {
				on_destroy.Push();
				hg_lua_OnDestroy(env.L(), -1);
			}
		});
		vm->Clear();
	}

	scene.Clear();
}

template <typename PhysicsT>
static void SceneClearSystemsWithPhysicsImpl(Scene &scene, PhysicsT &physics, SceneLuaVM *vm) {
	if (vm) {
		vm->ForeachScripts([](const LuaObject &env) {
			if (auto on_destroy = Get(env, "OnDestroy")) {
				on_destroy.Push();
				hg_lua_OnDestroy(env.L(), -1);
			}
		});
		vm->Clear();
	}

	physics.Clear();
	scene.Clear();
}

#if HG_ENABLE_BULLET3_SCENE_PHYSICS
void SceneSyncToSystemsFromFile(Scene &scene, SceneBullet3Physics &physics) {
	SceneSyncToSystemsWithPhysics(scene, physics, nullptr, g_file_reader, g_file_read_provider);
}
void SceneSyncToSystemsFromAssets(Scene &scene, SceneBullet3Physics &physics) {
	SceneSyncToSystemsWithPhysics(scene, physics, nullptr, g_assets_reader, g_assets_read_provider);
}
void SceneSyncToSystemsFromFile(Scene &scene, SceneBullet3Physics &physics, SceneLuaVM &vm) {
	SceneSyncToSystemsWithPhysics(scene, physics, &vm, g_file_reader, g_file_read_provider);
}
void SceneSyncToSystemsFromAssets(Scene &scene, SceneBullet3Physics &physics, SceneLuaVM &vm) {
	SceneSyncToSystemsWithPhysics(scene, physics, &vm, g_assets_reader, g_assets_read_provider);
}
#endif

void SceneUpdateSystems(Scene &scene, SceneClocks &clock, time_ns dt) { SceneUpdateSystemsNoPhysicsImpl(scene, clock, dt, nullptr); }
void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, SceneLuaVM &vm) {
	SceneUpdateSystemsNoPhysicsImpl(scene, clocks, dt, &vm);
}

#if HG_ENABLE_BULLET3_SCENE_PHYSICS || HG_ENABLE_TAU_SCENE_PHYSICS
void SceneSyncToSystemsFromFile(Scene &scene, ScenePhysics &physics) {
	SceneSyncToSystemsWithPhysics(scene, physics, nullptr, g_file_reader, g_file_read_provider);
}
void SceneSyncToSystemsFromAssets(Scene &scene, ScenePhysics &physics) {
	SceneSyncToSystemsWithPhysics(scene, physics, nullptr, g_assets_reader, g_assets_read_provider);
}
void SceneSyncToSystemsFromFile(Scene &scene, ScenePhysics &physics, SceneLuaVM &vm) {
	SceneSyncToSystemsWithPhysics(scene, physics, &vm, g_file_reader, g_file_read_provider);
}
void SceneSyncToSystemsFromAssets(Scene &scene, ScenePhysics &physics, SceneLuaVM &vm) {
	SceneSyncToSystemsWithPhysics(scene, physics, &vm, g_assets_reader, g_assets_read_provider);
}

void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, ScenePhysics &physics, time_ns physics_step, int physics_max_step) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, nullptr, physics_step, physics_max_step, nullptr);
}
void SceneUpdateSystems(
	Scene &scene, SceneClocks &clocks, time_ns dt, ScenePhysics &physics, time_ns physics_step, int physics_max_step, SceneLuaVM &vm) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, nullptr, physics_step, physics_max_step, &vm);
}
void SceneUpdateSystems(
	Scene &scene, SceneClocks &clocks, time_ns dt, ScenePhysics &physics, NodePairContacts &contacts, time_ns physics_step, int physics_max_step) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, &contacts, physics_step, physics_max_step, nullptr);
}
void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, ScenePhysics &physics, NodePairContacts &contacts, time_ns physics_step,
	int physics_max_step, SceneLuaVM &vm) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, &contacts, physics_step, physics_max_step, &vm);
}
#endif

#if HG_ENABLE_BULLET3_SCENE_PHYSICS
void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, SceneBullet3Physics &physics, time_ns physics_step, int physics_max_step) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, nullptr, physics_step, physics_max_step, nullptr);
}

void SceneUpdateSystems(
	Scene &scene, SceneClocks &clocks, time_ns dt, SceneBullet3Physics &physics, time_ns physics_step, int physics_max_step, SceneLuaVM &vm) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, nullptr, physics_step, physics_max_step, &vm);
}

void SceneUpdateSystems(
	Scene &scene, SceneClocks &clocks, time_ns dt, SceneBullet3Physics &physics, NodePairContacts &contacts, time_ns physics_step, int physics_max_step) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, &contacts, physics_step, physics_max_step, nullptr);
}

void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, SceneBullet3Physics &physics, NodePairContacts &contacts, time_ns physics_step,
	int physics_max_step, SceneLuaVM &vm) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, &contacts, physics_step, physics_max_step, &vm);
}
#endif

size_t SceneGarbageCollectSystems(Scene &scene) { return SceneGarbageCollectSystemsNoPhysicsImpl(scene, nullptr); }
size_t SceneGarbageCollectSystems(Scene &scene, SceneLuaVM &vm) { return SceneGarbageCollectSystemsNoPhysicsImpl(scene, &vm); }
#if HG_ENABLE_BULLET3_SCENE_PHYSICS || HG_ENABLE_TAU_SCENE_PHYSICS
size_t SceneGarbageCollectSystems(Scene &scene, ScenePhysics &physics) { return SceneGarbageCollectSystemsWithPhysicsImpl(scene, physics, nullptr); }
size_t SceneGarbageCollectSystems(Scene &scene, ScenePhysics &physics, SceneLuaVM &vm) {
	return SceneGarbageCollectSystemsWithPhysicsImpl(scene, physics, &vm);
}
#endif
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
size_t SceneGarbageCollectSystems(Scene &scene, SceneBullet3Physics &physics) { return SceneGarbageCollectSystemsWithPhysicsImpl(scene, physics, nullptr); }
size_t SceneGarbageCollectSystems(Scene &scene, SceneBullet3Physics &physics, SceneLuaVM &vm) {
	return SceneGarbageCollectSystemsWithPhysicsImpl(scene, physics, &vm);
}
#endif

void SceneClearSystems(Scene &scene) { SceneClearSystemsNoPhysicsImpl(scene, nullptr); }
void SceneClearSystems(Scene &scene, SceneLuaVM &vm) { SceneClearSystemsNoPhysicsImpl(scene, &vm); }
#if HG_ENABLE_BULLET3_SCENE_PHYSICS || HG_ENABLE_TAU_SCENE_PHYSICS
void SceneClearSystems(Scene &scene, ScenePhysics &physics) { SceneClearSystemsWithPhysicsImpl(scene, physics, nullptr); }
void SceneClearSystems(Scene &scene, ScenePhysics &physics, SceneLuaVM &vm) { SceneClearSystemsWithPhysicsImpl(scene, physics, &vm); }
#endif
#if HG_ENABLE_BULLET3_SCENE_PHYSICS
void SceneClearSystems(Scene &scene, SceneBullet3Physics &physics) { SceneClearSystemsWithPhysicsImpl(scene, physics, nullptr); }
void SceneClearSystems(Scene &scene, SceneBullet3Physics &physics, SceneLuaVM &vm) { SceneClearSystemsWithPhysicsImpl(scene, physics, &vm); }
#endif

#if HG_ENABLE_TAU_SCENE_PHYSICS
void SceneSyncToSystemsFromFile(Scene &scene, SceneTauPhysics &physics) {
	SceneSyncToSystemsWithPhysics(scene, physics, nullptr, g_file_reader, g_file_read_provider);
}
void SceneSyncToSystemsFromAssets(Scene &scene, SceneTauPhysics &physics) {
	SceneSyncToSystemsWithPhysics(scene, physics, nullptr, g_assets_reader, g_assets_read_provider);
}
void SceneSyncToSystemsFromFile(Scene &scene, SceneTauPhysics &physics, SceneLuaVM &vm) {
	SceneSyncToSystemsWithPhysics(scene, physics, &vm, g_file_reader, g_file_read_provider);
}
void SceneSyncToSystemsFromAssets(Scene &scene, SceneTauPhysics &physics, SceneLuaVM &vm) {
	SceneSyncToSystemsWithPhysics(scene, physics, &vm, g_assets_reader, g_assets_read_provider);
}

void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, SceneTauPhysics &physics, time_ns physics_step, int physics_max_step) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, nullptr, physics_step, physics_max_step, nullptr);
}
void SceneUpdateSystems(
	Scene &scene, SceneClocks &clocks, time_ns dt, SceneTauPhysics &physics, time_ns physics_step, int physics_max_step, SceneLuaVM &vm) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, nullptr, physics_step, physics_max_step, &vm);
}
void SceneUpdateSystems(
	Scene &scene, SceneClocks &clocks, time_ns dt, SceneTauPhysics &physics, NodePairContacts &contacts, time_ns physics_step, int physics_max_step) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, &contacts, physics_step, physics_max_step, nullptr);
}
void SceneUpdateSystems(Scene &scene, SceneClocks &clocks, time_ns dt, SceneTauPhysics &physics, NodePairContacts &contacts, time_ns physics_step,
	int physics_max_step, SceneLuaVM &vm) {
	SceneUpdateSystemsWithPhysicsImpl(scene, clocks, dt, physics, &contacts, physics_step, physics_max_step, &vm);
}

size_t SceneGarbageCollectSystems(Scene &scene, SceneTauPhysics &physics) { return SceneGarbageCollectSystemsWithPhysicsImpl(scene, physics, nullptr); }
size_t SceneGarbageCollectSystems(Scene &scene, SceneTauPhysics &physics, SceneLuaVM &vm) {
	return SceneGarbageCollectSystemsWithPhysicsImpl(scene, physics, &vm);
}

void SceneClearSystems(Scene &scene, SceneTauPhysics &physics) { SceneClearSystemsWithPhysicsImpl(scene, physics, nullptr); }
void SceneClearSystems(Scene &scene, SceneTauPhysics &physics, SceneLuaVM &vm) { SceneClearSystemsWithPhysicsImpl(scene, physics, &vm); }
#endif

} // namespace hg

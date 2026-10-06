// Physics overrides node matrix.
// Manual changes of position/rotation/scale of a physics node won't affect its matrix.

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	if (typeof hg.SceneBullet3Physics !== 'function') {
		throw Error('physics_overrides_matrix.js requires an hgjs build with Bullet scene physics. Use rebuild_hg_js_bullet.bat.');
	}
	return runWindow('Harfang - Physics Matrix Interaction', () => {
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		hg.ImGuiInit(10, hg.LoadProgramFromAssets('core/shader/imgui'), hg.LoadProgramFromAssets('core/shader/imgui_image'));

		// physics debug
		const vtx_line_layout = hg.VertexLayoutPosFloatColorUInt8();
		const line_shader = hg.LoadProgramFromAssets('shaders/pos_rgb');

		// create models
		const vtx_layout = hg.VertexLayoutPosFloatNormUInt8();

		const cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
		const cube_ref = res.AddModel('cube', cube_mdl);
		const ground_mdl = hg.CreateCubeModel(vtx_layout, 100, 0.02, 100);
		const ground_ref = res.AddModel('ground', ground_mdl);

		const prg_ref = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', res, hg.GetForwardPipelineInfo());

		// create material
		const mat = hg.CreateMaterial(prg_ref, 'uDiffuseColor', new hg.Vec4(0.5, 0.5, 0.5), 'uSpecularColor', new hg.Vec4(1, 1, 1));

		// setup scene
		const scene = new hg.Scene();

		const cam_mat = hg.TransformationMat4(new hg.Vec3(0, 1.5, -5), hg.Deg3(5, 0, 0));
		const cam = hg.CreateCamera(scene, cam_mat, 0.01, 1000);
		scene.SetCurrentCamera(cam);
		const view_matrix = hg.InverseFast(cam_mat);
		const c = cam.GetCamera();

		const lgt = hg.CreatePointLight(scene, hg.TranslationMat4(new hg.Vec3(3, 4, -6)), 0);

		const cube_node = hg.CreatePhysicCube(scene, new hg.Vec3(1, 1, 1), hg.TranslationMat4(new hg.Vec3(1.25, 2.5, 0)), cube_ref, [mat], 2);
		const ground_node = hg.CreatePhysicCube(scene, new hg.Vec3(100, 0.02, 100), hg.TranslationMat4(new hg.Vec3(0, -0.005, 0)), ground_ref, [mat], 0);

		const clocks = new hg.SceneClocks();

		// scene physics
		const physics = new hg.SceneBullet3Physics();
		scene.Update(0n);
		physics.SceneCreatePhysicsFromAssets(scene);

		// main loop
		const mouse = new hg.Mouse(), keyboard = new hg.Keyboard();

		return {
			draw(dt, res_x, res_y) {
				keyboard.Update();
				mouse.Update();

				// scene view
				let view_id = 0;
				hg.SceneUpdateSystems(scene, clocks, dt, physics, hg.time_from_sec_f(1 / 60), 1);
				[view_id] = hg.SubmitSceneToPipeline(view_id, scene, new hg.IntRect(0, 0, res_x, res_y), true, pipeline, res);

				// Debug physics display
				const projection_matrix = hg.ComputePerspectiveProjectionMatrix(c.GetZNear(), c.GetZFar(), hg.FovToZoomFactor(c.GetFov()), new hg.Vec2(res_x / res_y, 1));
				hg.SetViewClear(view_id, 0, hg.Color.Black, 1.0, 0);
				hg.SetViewRect(view_id, 0, 0, res_x, res_y);
				hg.SetViewTransform(view_id, view_matrix, projection_matrix);
				const rs = hg.ComputeRenderState(hg.BM_Opaque, hg.DT_Disabled, hg.FC_Disabled);
				physics.RenderCollision(view_id, vtx_line_layout, line_shader, rs, 0);

				// ImGui view
				hg.ImGuiBeginFrame(res_x, res_y, dt > 0n ? dt : 1n, mouse.GetState(), keyboard.GetState());

				const [visible] = hg.ImGuiBegin('Transform and Physics', true, hg.ImGuiWindowFlags_AlwaysAutoResize);
				if (visible) {
					hg.ImGuiTextWrapped('This tutorial demonstrates the interaction between the physics system and the Transform component of a node. The node position, rotation and scale are overriden by an active node rigid body.');

					hg.ImGuiSeparator();

					const [r, v] = hg.ImGuiInputVec3('Transform.pos', cube_node.GetTransform().GetPos(), 2);
					if (r) {
						cube_node.GetTransform().SetPos(v);
					}

					if (hg.ImGuiButton('Press to reset position using Transform.SetPos')) {
						cube_node.GetTransform().SetPos(new hg.Vec3(1.25, 2.5, 0));
					}

					hg.ImGuiSeparator();

					hg.ImGuiInputVec3('Transform.GetWorld().T', hg.GetT(cube_node.GetTransform().GetWorld()), 2, hg.ImGuiInputTextFlags_ReadOnly);

					if (physics.NodeHasBody(cube_node)) {
						if (hg.ImGuiButton('Press to destroy the cube node physics')) {
							physics.NodeDestroyPhysics(cube_node);
							physics.GarbageCollect(scene);
						}
					} else {
						if (hg.ImGuiButton('Create to create the cube node physics')) {
							physics.NodeCreatePhysicsFromAssets(cube_node);
						}
					}
				}

				hg.ImGuiEnd();

				hg.ImGuiEndFrame(255);
			},

			// cleanup
			dispose() {
				hg.ImGuiShutdown();
				hg.SceneClearSystems(scene, physics);
				hg.DestroyProgram(line_shader);
				res.DestroyAllTextures();
				res.DestroyAllModels();
				res.DestroyAllPrograms();
				hg.DestroyForwardPipeline(pipeline);
			},
		};
	}, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}

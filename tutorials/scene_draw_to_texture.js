// Draw scene to texture

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Draw Scene to Texture', () => {
		// create pipeline
		const pipeline = hg.CreateForwardPipeline();
		const res = new hg.PipelineResources();

		// load the scene to draw to a texture
		const scene = new hg.Scene();
		if (!hg.LoadSceneFromAssets('materials/materials.scn', scene, res, hg.GetForwardPipelineInfo())) {
			throw Error('Failed to load materials/materials.scn');
		}

		// create a 512x512 frame buffer to draw the scene to
		const frame_buffer = hg.CreateFrameBuffer(512, 512, hg.TF_RGBA32F, hg.TF_D24, 4, 'framebuffer'); // 4x MSAA
		if (!hg.IsValid(frame_buffer)) {
			throw Error('Failed to create framebuffer');
		}
		const color = hg.GetColorTexture(frame_buffer);

		// create the cube model
		const vtx_layout = hg.VertexLayoutPosFloatTexCoord0UInt8();

		const cube_mdl = hg.CreateCubeModel(vtx_layout, 1, 1, 1);
		const cube_ref = res.AddModel('cube', cube_mdl);

		// prepare the cube shader program
		const cube_prg = hg.LoadProgramFromAssets('shaders/texture');

		// main loop
		let angle = 0;

		return {
			draw(dt, res_x, res_y) {
				angle += hg.time_to_sec_f(dt);

				// update scene and render to the frame buffer
				scene.GetCurrentCamera().GetTransform().SetPos(new hg.Vec3(0, 0, -(Math.sin(angle) * 3 + 4))); // animate the scene current camera on Z

				scene.Update(dt);

				let view_id = 0;
				[view_id] = hg.SubmitSceneToPipeline(view_id, scene, new hg.IntRect(0, 0, 512, 512), true, pipeline, res, frame_buffer.handle);

				// draw a rotating cube in immediate mode using the texture the scene was rendered to
				hg.SetViewPerspective(view_id, 0, 0, res_x, res_y, hg.TranslationMat4(new hg.Vec3(0, 0, -1.8)));

				const val_uniforms = [hg.MakeUniformSetValue('color', new hg.Vec4(1, 1, 1, 1))]; // note: these could be moved out of the main loop but are kept here for readability
				const tex_uniforms = [hg.MakeUniformSetTexture('s_tex', color, 0)];

				hg.DrawModel(view_id, cube_mdl, cube_prg, val_uniforms, tex_uniforms, hg.TransformationMat4(new hg.Vec3(0, 0, 0), new hg.Vec3(angle * 0.1, angle * 0.05, angle * 0.2)));
			},

			// cleanup
			dispose() {
				scene.Clear();
				hg.DestroyFrameBuffer(frame_buffer); // also releases its color and depth textures
				hg.DestroyProgram(cube_prg);
				res.DestroyAllTextures();
				res.DestroyAllModels();
				res.DestroyAllPrograms();
				hg.DestroyForwardPipeline(pipeline);
			},
		};
	}, { width: 1024, height: 1024, resetFlags: hg.RF_VSync | hg.RF_MSAA8X, ...options });
}

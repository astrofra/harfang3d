// Starfield 3D

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
	return runWindow('Harfang - Starfield', () => {
		// vertex layout
		const vtx_layout = new hg.VertexLayout();
		vtx_layout.Begin();
		vtx_layout.Add(hg.A_Position, 3, hg.AT_Float);
		vtx_layout.Add(hg.A_Color0, 3, hg.AT_Float);
		vtx_layout.End();

		// simple shader program
		const shader = hg.LoadProgramFromAssets('shaders/pos_rgb');

		// initialize stars
		const starfield_size = 10;

		const max_stars = 1000;
		const vtx = new hg.Vertices(vtx_layout, max_stars * 2);
		// Keep depth positive: perspective division must never cross zero.
		const near = 0.1;
		const stars = [];
		for (let i = 0; i < max_stars; i++) {
			stars.push(new hg.Vec3(
				(Math.random() * 2 - 1) * starfield_size,
				(Math.random() * 2 - 1) * starfield_size,
				near + Math.random() * starfield_size));
		}

		// main loop
		return {
			draw(dt, width, height) {
				hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color.Black, 1, 0);
				hg.SetViewRect(0, 0, 0, width, height);

				const dt_f = hg.time_to_sec_f(dt);

				// update stars
				vtx.Clear();
				for (let i = 0; i < stars.length; i++) {
					const star = stars[i];
					// Modulo also handles a frame long enough to cross the whole field.
					star.z = near + ((star.z - near - 2 * dt_f) % starfield_size + starfield_size) % starfield_size;
					const x = star.x / star.z, y = star.y / star.z;
					vtx.Begin(2 * i).SetPos(new hg.Vec3(x, y, 0)).SetColor0(hg.Color.Black).End();
					vtx.Begin(2 * i + 1).SetPos(new hg.Vec3(x * 1.04, y * 1.04, 0)).SetColor0(hg.Color.White).End();
				}

				// draw stars as lines
				hg.DrawLines(0, vtx, shader);
			},

			// cleanup
			dispose() {
				hg.DestroyProgram(shader);
			},
		};
	}, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}

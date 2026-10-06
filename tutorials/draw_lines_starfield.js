// Starfield: project 1,000 moving stars and batch their trails in one draw call.

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
  return runWindow('HarfangJs - Starfield', () => {
    const layout = new hg.VertexLayout();
    layout.Begin();
    layout.Add(hg.A_Position, 3, hg.AT_Float);
    layout.Add(hg.A_Color0, 3, hg.AT_Float);
    layout.End();

    const shader = hg.LoadProgramFromAssets('shaders/pos_rgb');
    const starfieldSize = 10;
    const starCount = 1000;
    const vertices = new hg.Vertices(layout, starCount * 2);
    // Keep depth positive: perspective division must never cross zero.
    const near = 0.1;
    const stars = Array.from({ length: starCount }, () => new hg.Vec3(
      (Math.random() * 2 - 1) * starfieldSize,
      (Math.random() * 2 - 1) * starfieldSize,
      near + Math.random() * starfieldSize));

    return {
      draw(dt, width, height) {
        hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.Color.Black, 1, 0);
        hg.SetViewRect(0, 0, 0, width, height);
        const distance = 2 * hg.time_to_sec_f(dt);
        vertices.Clear();
        for (let i = 0; i < stars.length; i++) {
          const star = stars[i];
          // Modulo also handles a frame long enough to cross the whole field.
          star.z = near + ((star.z - near - distance) % starfieldSize + starfieldSize) % starfieldSize;
          const x = star.x / star.z, y = star.y / star.z;
          vertices.Begin(2 * i).SetPos(new hg.Vec3(x, y, 0)).SetColor0(hg.Color.Black).End();
          vertices.Begin(2 * i + 1).SetPos(new hg.Vec3(x * 1.04, y * 1.04, 0)).SetColor0(hg.Color.White).End();
        }
        hg.DrawLines(0, vertices, shader);
      },

      dispose() {
        hg.DestroyProgram(shader);
      },
    };
  }, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}

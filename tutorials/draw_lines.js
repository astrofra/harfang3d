// Port of draw_lines.lua / draw_lines.nut: 1,000 animated line segments.
import * as hg from 'harfang';
import {runWindow} from './js/window.js';

export function main(options) {
  return runWindow('HarfangJs - Draw Lines', () => {
    const shader = hg.LoadProgramFromAssets('shaders/white');
    const layout = new hg.VertexLayout();
    layout.Begin(); layout.Add(hg.A_Position, 3, hg.AT_Float); layout.End();
    const lineCount = 1000, vertices = new hg.Vertices(layout, lineCount * 2);
    let angle = 0;
    return {
      draw(dt, width, height) {
        hg.SetViewClear(0, hg.CF_Color | hg.CF_Depth, hg.ColorI(64, 64, 64), 1, 0);
        hg.SetViewRect(0, 0, 0, width, height);
        vertices.Clear();
        for (let i = 0; i < lineCount; i++) {
          vertices.Begin(2 * i).SetPos(new hg.Vec3(Math.sin(angle + i * .005), Math.cos(angle + i * .01), 0)).End();
          vertices.Begin(2 * i + 1).SetPos(new hg.Vec3(Math.sin(angle - i * .005), Math.cos(angle + i * .005), 0)).End();
        }
        hg.DrawLines(0, vertices, shader);
        angle += hg.time_to_sec_f(dt);
      },
      dispose() { hg.DestroyProgram(shader); },
    };
  }, options);
}

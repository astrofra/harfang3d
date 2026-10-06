// Draw centered text with a font atlas, alpha blending and a color uniform.

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
  return runWindow('HarfangJs - Draw Text', () => {
    const font = hg.LoadFontFromAssets('font/default.ttf', 96);
    const shader = hg.LoadProgramFromAssets('core/shader/font');
    const values = [hg.MakeUniformSetValue('u_color', new hg.Vec4(1, 1, 0))];
    const state = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Always, hg.FC_Disabled);

    return {
      draw(dt, width, height) {
        hg.SetView2D(0, 0, 0, width, height, -1, 1, hg.CF_Color | hg.CF_Depth, hg.ColorI(32, 32, 32), 0, 1);
        hg.DrawText(0, font, 'Hello world!', shader, 'u_tex', 0, hg.Mat4.Identity,
          new hg.Vec3(width / 2, height / 2, 0), hg.DTHA_Center, hg.DTVA_Center, values, [], state);
      },

      dispose() {
        hg.DestroyProgram(shader);
        // Font atlas textures are released by RenderShutdown (no DestroyFont API).
      },
    };
  }, { resetFlags: hg.RF_VSync, ...options });
}

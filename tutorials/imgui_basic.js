// ImGui basics: begin a frame, build a window, then submit its draw commands.

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
  return runWindow('HarfangJs - ImGui Basics', () => {
    const shader = hg.LoadProgramFromAssets('core/shader/imgui');
    const imageShader = hg.LoadProgramFromAssets('core/shader/imgui_image');
    hg.ImGuiInit(10, shader, imageShader);

    return {
      draw(dt, width, height) {
        hg.ImGuiBeginFrame(width, height, dt > 0n ? dt : 1n, hg.ReadMouse(), hg.ReadKeyboard());
        if (hg.ImGuiBegin('Window')) {
          hg.ImGuiText('Hello World!');
        }
        // End is required even when Begin returns false (a collapsed window).
        hg.ImGuiEnd();

        hg.SetView2D(0, 0, 0, width, height, -1, 1, hg.CF_Color | hg.CF_Depth, hg.Color.Black, 1, 0);
        hg.ImGuiEndFrame(0);
      },

      dispose() {
        // ImGui owns both programs and destroys them with its font texture.
        hg.ImGuiShutdown();
      },
    };
  }, { resetFlags: hg.RF_VSync, resizeToWindow: true, ...options });
}

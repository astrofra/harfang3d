// Draw models without a pipeline

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
  return runWindow('HarfangJs - Draw Models no Pipeline', () => {
    // vertex layout and models
    const layout = hg.VertexLayoutPosFloatNormUInt8();

    const cube = hg.CreateCubeModel(layout, 1, 1, 1);
    const ground = hg.CreatePlaneModel(layout, 5, 5, 1, 1);

    const shader = hg.LoadProgramFromAssets('shaders/mdl');

    // Own the models' GPU buffers.
    const resources = new hg.PipelineResources();
    resources.AddModel('cube', cube);
    resources.AddModel('ground', ground);

    // main loop
    let angle = 0;

    return {
      draw(dt, width, height) {
        angle += hg.time_to_sec_f(dt);

        hg.SetViewPerspective(0, 0, 0, width, height, hg.TranslationMat4(new hg.Vec3(0, 1, -3)));

        // C++ vector parameters accept JS arrays.
        hg.DrawModel(0, cube, shader, [], [], hg.TransformationMat4(new hg.Vec3(0, 1, 0), new hg.Vec3(angle)));
        hg.DrawModel(0, ground, shader, [], [], hg.TranslationMat4(new hg.Vec3()));
      },

      // cleanup
      dispose() {
        resources.DestroyAllModels();
        hg.DestroyProgram(shader);
      },
    };
  }, options);
}

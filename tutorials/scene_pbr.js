// Scene using the PBR shader

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
  return runWindow('HarfangJs - PBR Scene', () => {
    const pipeline = hg.CreateForwardPipeline();
    const resources = new hg.PipelineResources();

    // access to compiled resources is set up by runWindow

    // load scene
    const scene = new hg.Scene();
    if (!hg.LoadSceneFromAssets('materials/materials.scn', scene, resources, hg.GetForwardPipelineInfo())) {
      throw Error('Failed to load materials/materials.scn');
    }

    // main loop
    return {
      draw(dt, width, height) {
        scene.Update(dt);
        hg.SubmitSceneToPipeline(0, scene, new hg.IntRect(0, 0, width, height), true, pipeline, resources);
      },

      // cleanup
      dispose() {
        scene.Clear();
        resources.DestroyAllTextures();
        resources.DestroyAllModels();
        resources.DestroyAllPrograms();
        hg.DestroyForwardPipeline(pipeline);
      },
    };
  }, { width: 940, resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}

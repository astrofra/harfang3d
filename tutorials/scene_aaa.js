// Toyota 2JZ-GTE Engine model by Serhii Denysenko (CGTrader: serhiidenysenko8256).
// https://www.cgtrader.com/3d-models/vehicle/part/toyota-2jz-gte-engine-2932b715-2f42-4ecd-93ce-df9507c67ce8
// Native HARFANG uses AAA; HG JS Web implements these calls as forward stubs.

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main({ aaa = true, ...options } = {}) {
  return runWindow('HarfangJs - AAA Scene', () => {
    const pipeline = hg.CreateForwardPipeline();
    const resources = new hg.PipelineResources();
    const scene = new hg.Scene();
    if (!hg.LoadSceneFromAssets('car_engine/engine.scn', scene, resources, hg.GetForwardPipelineInfo())) {
      throw Error('Failed to load car_engine/engine.scn');
    }
    const config = new hg.ForwardPipelineAAAConfig();
    const pipelineAAA = aaa ? hg.CreateForwardPipelineAAAFromAssets('core', config, hg.BR_Equal, hg.BR_Equal) : undefined;
    config.sample_count = 1;
    const transform = scene.GetNode('engine_master').GetTransform();
    if (!transform.IsValid()) throw Error('Missing engine_master transform');
    let frame = 0;

    return {
      draw(dt, width, height) {
        // This scripted rotation remains active when animation playback is stubbed.
        transform.SetRot(transform.GetRot().add(new hg.Vec3(0, hg.Deg(15) * hg.time_to_sec_f(dt), 0)));
        scene.Update(dt);
        const args = [0, scene, new hg.IntRect(0, 0, width, height), true, pipeline, resources];
        if (aaa) hg.SubmitSceneToPipeline(...args, pipelineAAA, config, frame);
        else hg.SubmitSceneToPipeline(...args);
        ++frame; // runWindow owns the single Frame() call for each iteration.
      },
      dispose() {
        scene.Clear();
        if (pipelineAAA) hg.DestroyForwardPipelineAAA(pipelineAAA);
        resources.DestroyAllTextures();
        resources.DestroyAllModels();
        resources.DestroyAllPrograms();
        hg.DestroyForwardPipeline(pipeline);
      },
    };
  }, { width: 1280, height: 720, resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}

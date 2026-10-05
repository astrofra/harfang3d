// Many dynamic objects

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
  return runWindow('HarfangJs - Many dynamic objects', () => {
    const pipeline = hg.CreateForwardPipeline(4096); // increase shadow map resolution to 4096x4096
    const resources = new hg.PipelineResources();

    // create models
    const layout = hg.VertexLayoutPosFloatNormUInt8();

    const sphereModel = hg.CreateSphereModel(layout, 0.1, 8, 16);
    const sphereRef = resources.AddModel('sphere', sphereModel);
    const groundModel = hg.CreateCubeModel(layout, 60, 0.001, 60);
    const groundRef = resources.AddModel('ground', groundModel);

    // create materials
    const shader = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', resources, hg.GetForwardPipelineInfo());

    const sphereMaterial = hg.CreateMaterial(shader, 'uDiffuseColor', new hg.Vec4(1, 0, 0), 'uSpecularColor', new hg.Vec4(1, 0.8, 0));
    const groundMaterial = hg.CreateMaterial(shader, 'uDiffuseColor', new hg.Vec4(1, 1, 1), 'uSpecularColor', new hg.Vec4(1, 1, 1));

    // setup scene
    const scene = new hg.Scene();
    scene.canvas.color = new hg.Color(0.1, 0.1, 0.1);
    scene.environment.ambient = new hg.Color(0.1, 0.1, 0.1);

    const camera = hg.CreateCamera(
      scene, hg.TransformationMat4(new hg.Vec3(15.5, 5, -6), new hg.Vec3(0.4, -1.2, 0)), 0.01, 100);
    scene.SetCurrentCamera(camera);

    hg.CreateSpotLight(
      scene, hg.TransformationMat4(new hg.Vec3(-8.8, 21.7, -8.8), hg.Deg3(60, 45, 0)),
      0, hg.Deg(5), hg.Deg(30), hg.Color.White, hg.Color.White, 0, hg.LST_Map, 0.000005);
    hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(0, 0, 0)), groundRef, [groundMaterial]);

    // create scene objects
    const rows = [];
    for (let z = -100; z <= 100; z += 2) {
      const row = [];
      for (let x = -100; x <= 100; x += 2) {
        const node = hg.CreateObject(
          scene, hg.TranslationMat4(new hg.Vec3(x * 0.1, 0.1, z * 0.1)), sphereRef, [sphereMaterial]);
        row.push(node.GetTransform()); // store the node transform directly
      }
      rows.push(row);
    }

    // main loop
    let angle = 0;

    return {
      draw(dt, width, height) {
        angle += hg.time_to_sec_f(dt);

        // Lua arrays start at 1; preserve the same wave phase in JavaScript.
        for (let j = 0; j < rows.length; j++) {
          const row = rows[j];
          const rowY = Math.cos(angle + (j + 1) * 0.1);
          for (let i = 0; i < row.length; i++) {
            const transform = row[i];
            const position = transform.GetPos();
            position.y = 0.1 * (rowY * Math.sin(angle + (i + 1) * 0.1) * 6 + 6.5);
            transform.SetPos(position);
          }
        }

        scene.Update(dt);

        hg.SubmitSceneToPipeline(0, scene, new hg.IntRect(0, 0, width, height), true, pipeline, resources);
      },

      // cleanup
      dispose() {
        rows.length = 0;
        scene.Clear();
        resources.DestroyAllTextures();
        resources.DestroyAllModels();
        resources.DestroyAllPrograms();
        hg.DestroyForwardPipeline(pipeline);
      },
    };
  }, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}

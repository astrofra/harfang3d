// Render one scene from four viewpoints, sharing the shadow-map preparation.

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
  return runWindow('HarfangJs - Scene Draw to Multiple Viewports', () => {
    const pipeline = hg.CreateForwardPipeline();
    const resources = new hg.PipelineResources();
    const layout = hg.VertexLayoutPosFloatNormUInt8();
    const cube = resources.AddModel('cube', hg.CreateCubeModel(layout, 1, 1, 1));
    const ground = resources.AddModel('ground', hg.CreateCubeModel(layout, 100, 0.01, 100));
    const shader = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', resources, hg.GetForwardPipelineInfo());
    const yellow = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(255, 220, 64), 'uSpecularColor', hg.Vec4I(255, 220, 64));
    const red = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(255, 0, 0), 'uSpecularColor', hg.Vec4I(255, 0, 0));
    const grey = hg.CreateMaterial(shader, 'uDiffuseColor', hg.Vec4I(128, 128, 128), 'uSpecularColor', hg.Vec4I(128, 128, 128));

    // No camera node: each viewport supplies its own view state.
    const scene = new hg.Scene();
    hg.CreateSpotLight(scene, hg.TransformationMat4(new hg.Vec3(-8, 4, -5), hg.Deg3(19, 59, 0)),
      0, hg.Deg(5), hg.Deg(30), hg.Color.White, hg.Color.White, 10, hg.LST_Map, 0.00005);
    hg.CreatePointLight(scene, hg.TranslationMat4(new hg.Vec3(3, 1, 2.5)), 5, hg.ColorI(128, 192, 255), hg.Color.Black, 0);
    const yellowCube = hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(1, 0.5, 0)), cube, [yellow]);
    hg.CreateObject(scene, hg.TranslationMat4(new hg.Vec3(-1, 0.5, 0)), cube, [red]);
    hg.CreateObject(scene, hg.Mat4.Identity, ground, [grey]);

    const cameras = [
      hg.TransformationMat4(new hg.Vec3(-4.015, 2.368, -3.484), new hg.Vec3(0.35, 0.87, 0)),
      hg.TransformationMat4(new hg.Vec3(-4.143, 2.976, 4.127), new hg.Vec3(0.423, 2.365, 0)),
      hg.TransformationMat4(new hg.Vec3(4.020, 2.374, 3.469), new hg.Vec3(0.353, 4.016, 0)),
      hg.TransformationMat4(new hg.Vec3(3.469, 2.374, -4.020), new hg.Vec3(0.353, -0.695, 0)),
    ];

    return {
      draw(dt, width, height) {
        const transform = yellowCube.GetTransform();
        const rotation = transform.GetRot();
        rotation.y += hg.time_to_sec_f(dt);
        transform.SetRot(rotation);
        scene.Update(dt);

        const halfWidth = Math.floor(width / 2), halfHeight = Math.floor(height / 2);
        const rects = [
          new hg.IntRect(0, 0, halfWidth, halfHeight),
          new hg.IntRect(halfWidth, 0, width, halfHeight),
          new hg.IntRect(0, halfHeight, halfWidth, height),
          new hg.IntRect(halfWidth, halfHeight, width, height),
        ];
        const renderData = new hg.SceneForwardPipelineRenderData();
        let views = new hg.SceneForwardPipelinePassViewId();
        // Native in/out parameters are returned as a JavaScript array.
        let viewId;
        [viewId, views] = hg.PrepareSceneForwardPipelineCommonRenderData(0, scene, renderData, pipeline, resources, views);
        for (let i = 0; i < cameras.length; i++) {
          const rect = rects[i];
          const viewState = hg.ComputePerspectiveViewState(cameras[i], hg.Deg(45), 0.01, 1000,
            hg.ComputeAspectRatioX(rect.ex - rect.sx, rect.ey - rect.sy));
          [viewId, views] = hg.PrepareSceneForwardPipelineViewDependentRenderData(
            viewId, viewState, scene, renderData, pipeline, resources, views);
          [viewId] = hg.SubmitSceneToForwardPipeline(viewId, scene, rect, viewState, pipeline, renderData, resources);
        }
      },

      dispose() {
        scene.Clear();
        resources.DestroyAllTextures();
        resources.DestroyAllModels();
        resources.DestroyAllPrograms();
        hg.DestroyForwardPipeline(pipeline);
      },
    };
  }, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}

// Mouse flight

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options) {
  return runWindow('HarfangJs - Mouse Flight', () => {
    const resources = new hg.PipelineResources();
    const pipeline = hg.CreateForwardPipeline();

    const mouse = new hg.Mouse();

    // access to compiled resources is set up by runWindow

    // 2D drawing helpers
    const layout = hg.VertexLayoutPosFloatColorFloat();

    const draw2DProgram = hg.LoadProgramFromAssets('shaders/pos_rgb');
    const draw2DRenderState = hg.ComputeRenderState(hg.BM_Alpha, hg.DT_Less, hg.FC_Disabled);

    function drawCircle(viewId, center, radius, color) {
      const segmentCount = 32;
      const step = 2 * Math.PI / segmentCount;
      const p0 = new hg.Vec3(center.x + radius, center.y, 0);
      const p1 = new hg.Vec3(0, 0, 0);

      const vertices = new hg.Vertices(layout, segmentCount * 2 + 2);

      for (let i = 0; i <= segmentCount; i++) {
        p1.x = radius * Math.cos(i * step) + center.x;
        p1.y = radius * Math.sin(i * step) + center.y;
        vertices.Begin(2 * i).SetPos(p0).SetColor0(color).End();
        vertices.Begin(2 * i + 1).SetPos(p1).SetColor0(color).End();
        p0.x = p1.x;
        p0.y = p1.y;
      }

      hg.DrawLines(viewId, vertices, draw2DProgram, draw2DRenderState);
    }

    // gameplay settings
    const cameraChaseOffset = new hg.Vec3(0, 0.2, 0);
    const cameraChaseDistance = 1;

    const planeSpeed = 0.05;
    const planeMouseSensitivity = 0.5;

    // setup game world
    const scene = new hg.Scene();
    if (!hg.LoadSceneFromAssets('playground/playground.scn', scene, resources, hg.GetForwardPipelineInfo())) {
      throw Error('Failed to load playground/playground.scn');
    }

    // Multiple return values (node and load status) form a JavaScript array.
    const [planeNode, planeLoaded] = hg.CreateInstanceFromAssets(
      scene, hg.TranslationMat4(new hg.Vec3(0, 4, 0)),
      'paper_plane/paper_plane.scn', resources, hg.GetForwardPipelineInfo());

    if (!planeLoaded) {
      throw Error('Failed to load paper_plane/paper_plane.scn');
    }

    const cameraNode = hg.CreateCamera(scene, hg.TranslationMat4(new hg.Vec3(0, 4, -5)), 0.01, 1000);

    scene.SetCurrentCamera(cameraNode);

    function updatePlane(mouseXNormalized, mouseYNormalized) {
      const transform = planeNode.GetTransform();

      // Native vector operators use methods in JavaScript.
      const position = transform.GetPos().add(hg.Normalize(hg.GetZ(transform.GetWorld())).mul(planeSpeed));
      position.y = hg.Clamp(position.y, 0.1, 50); // floor/ceiling

      const rotation = transform.GetRot();

      const nextRotation = new hg.Vec3(rotation); // make a copy of the plane rotation
      nextRotation.x = hg.Clamp(nextRotation.x + mouseYNormalized * -0.03, -0.75, 0.75);
      nextRotation.y += mouseXNormalized * 0.03;
      nextRotation.z = hg.Clamp(mouseXNormalized * -0.75, -1.2, 1.2);

      const smoothedRotation = rotation.add(nextRotation.sub(rotation).mul(planeMouseSensitivity));

      transform.SetPos(position);
      transform.SetRot(smoothedRotation);
    }

    function updateChaseCamera(targetPosition) {
      const transform = cameraNode.GetTransform();
      const cameraToTarget = hg.Normalize(targetPosition.sub(transform.GetPos()));

      // Keep the camera at the chase distance from its target.
      transform.SetPos(targetPosition.sub(cameraToTarget.mul(cameraChaseDistance)));
      transform.SetRot(hg.ToEuler(hg.Mat3LookAt(cameraToTarget)));
    }

    // game loop
    return {
      draw(dt, width, height) {
        // update mouse device; runWindow handles the keyboard's Escape key
        mouse.Update();

        // compute ratio corrected normalized mouse position
        const mouseX = mouse.X();
        const mouseY = mouse.Y();

        const aspectRatio = hg.ComputeAspectRatioX(width, height);
        const mouseXNormalized = (mouseX / width - 0.5) * aspectRatio.x;
        const mouseYNormalized = (mouseY / height - 0.5) * aspectRatio.y;

        // update gameplay elements (plane & camera)
        updatePlane(mouseXNormalized, mouseYNormalized);
        updateChaseCamera(planeNode.GetTransform().GetWorld().mul(cameraChaseOffset));

        // update scene and submit it to render pipeline
        scene.Update(dt);

        const [viewId] = hg.SubmitSceneToPipeline(
          0, scene, new hg.IntRect(0, 0, width, height), true, pipeline, resources);

        // draw 2D GUI
        hg.SetView2D(viewId, 0, 0, width, height, -1, 1, hg.CF_Depth, hg.Color.Black, 1, 0, true);
        drawCircle(viewId, new hg.Vec3(mouseX, mouseY, 0), 20, hg.Color.White); // display mouse cursor
      },

      // cleanup
      dispose() {
        scene.Clear();
        resources.DestroyAllTextures();
        resources.DestroyAllModels();
        resources.DestroyAllPrograms();
        hg.DestroyProgram(draw2DProgram);
        hg.DestroyForwardPipeline(pipeline);
      },
    };
  }, { resetFlags: hg.RF_VSync | hg.RF_MSAA8X, ...options });
}

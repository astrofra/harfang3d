// Keep a cube above the ground using a force or an impulse. Space switches mode.

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main({ useForce = true, ...options } = {}) {
  if (typeof hg.SceneBullet3Physics !== 'function') {
    throw Error('physics_impulse.js requires an hgjs build with Bullet scene physics. Use rebuild_hg_js_bullet.bat.');
  }
  return runWindow('HarfangJs - Physics Force/Impulse (Space to alternate)', () => {
    const pipeline = hg.CreateForwardPipeline();
    const resources = new hg.PipelineResources();
    const layout = hg.VertexLayoutPosFloatNormUInt8();
    const cube = resources.AddModel('cube', hg.CreateCubeModel(layout, 1, 1, 1));
    const ground = resources.AddModel('ground', hg.CreateCubeModel(layout, 50, 0.01, 50));
    const shader = hg.LoadPipelineProgramRefFromAssets('core/shader/default.hps', resources, hg.GetForwardPipelineInfo());
    const material = hg.CreateMaterial(shader, 'uDiffuseColor', new hg.Vec4(1, 1, 1), 'uSpecularColor', new hg.Vec4(1, 1, 1));

    const scene = new hg.Scene();
    const camera = hg.CreateCamera(scene,
      hg.TransformationMat4(new hg.Vec3(0, 1.5, -5), hg.Deg3(10, 0, 0)), 0.01, 1000);
    scene.SetCurrentCamera(camera);
    hg.CreateLinearLight(scene, hg.TransformationMat4(new hg.Vec3(), hg.Deg3(30, 59, 0)),
      hg.Color.White, hg.Color.White, 10, hg.LST_Map, 0.002, new hg.Vec4(2, 4, 10, 16));
    const cubeNode = hg.CreatePhysicCube(scene, new hg.Vec3(1, 1, 1),
      hg.TranslationMat4(new hg.Vec3(0, 2.5, 0)), cube, [material], 2);
    hg.CreatePhysicCube(scene, new hg.Vec3(100, 0.02, 100),
      hg.TranslationMat4(new hg.Vec3(0, -0.005, 0)), ground, [material], 0);

    const clocks = new hg.SceneClocks();
    const physics = new hg.SceneBullet3Physics();
    scene.Update(0n);
    physics.SceneCreatePhysicsFromAssets(scene);
    const physicsStep = hg.time_from_sec_f(1 / 60);
    const keyboard = new hg.Keyboard();
    console.log(`Mode: ${useForce ? 'force' : 'impulse'} (Space to switch)`);

    return {
      draw(dt, width, height) {
        keyboard.Update();
        if (keyboard.Pressed(hg.K_Space)) {
          useForce = !useForce;
          console.log(`Mode: ${useForce ? 'force' : 'impulse'}`);
        }

        const position = hg.GetT(cubeNode.GetTransform().GetWorld());
        const distance = position.y - 0.5;
        if (distance < 1) {
          const k = 1 - distance;
          if (useForce) {
            physics.NodeAddForce(cubeNode, new hg.Vec3(0, k * 80, 0), position);
          } else {
            const velocity = physics.NodeGetLinearVelocity(cubeNode);
            const targetVelocity = new hg.Vec3(0, k * 10, 0);
            // NodeAddImpulse accepts an instantaneous velocity change, as in Lua.
            physics.NodeAddImpulse(cubeNode, targetVelocity.sub(velocity), position);
          }
        }
        physics.NodeWake(cubeNode);

        // Bound long pauses while keeping the native fixed physics step (BigInt ns).
        hg.SceneUpdateSystems(scene, clocks, dt > 50000000n ? 50000000n : dt, physics, physicsStep, 3);
        hg.SubmitSceneToPipeline(0, scene, new hg.IntRect(0, 0, width, height), true, pipeline, resources);
      },

      dispose() {
        hg.SceneClearSystems(scene, physics);
        resources.DestroyAllTextures();
        resources.DestroyAllModels();
        resources.DestroyAllPrograms();
        hg.DestroyForwardPipeline(pipeline);
      },
    };
  }, { resetFlags: hg.RF_VSync | hg.RF_MSAA4X, ...options });
}

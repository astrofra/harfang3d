// Shared window/teardown code for the native JavaScript tutorials.
// Tests call the same main() with a hidden window and a finite frame count.

import * as hg from 'harfang';
import { nextFrame } from 'harfang-host';

export async function runWindow(title, create, {
  hidden = false,
  frameLimit = Infinity,
  capturePath,
  renderer,
} = {}) {
  const width = 1280;
  const height = 720;

  // As in the Lua/Squirrel tutorials, the application chooses its compiled assets.
  hg.AddAssetsFolder('resources_compiled');

  // input and window setup
  hg.InputInit();
  hg.WindowSystemInit();

  let window;
  let initialized = false;
  let app;

  try {
    window = hg.NewWindow(title, width, height, 32, hidden ? hg.WV_Hidden : hg.WV_Windowed);

    // Use the same renderer selection as Lua unless the caller explicitly
    // selects a backend matching its compiled shaders (e.g. GL capture tests).
    const ready = renderer === undefined ? hg.RenderInit(window) : hg.RenderInit(window, renderer);
    if (!ready) {
      throw Error('Renderer initialization failed');
    }

    initialized = true;
    app = create();

    // main loop
    for (let frame = 0; frame < frameLimit; frame++) {
      const state = await nextFrame(window);
      if (state.closed || hg.ReadKeyboard().Key(hg.K_Escape)) {
        break;
      }

      app.draw(state.dtNs, width, height);

      if (capturePath && frame === 3) {
        hg.RequestScreenShot(hg.InvalidFrameBufferHandle, capturePath);
      }

      hg.Frame();
    }
  } finally {
    // cleanup
    try {
      if (app) {
        app.dispose();
      }
    } finally {
      if (initialized) {
        hg.RenderShutdown();
      }

      if (window) {
        hg.DestroyWindow(window);
      }

      hg.WindowSystemShutdown();
      hg.InputShutdown();
    }
  }
}

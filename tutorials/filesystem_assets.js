// Access to a file by mounting a folder as an assets source

import * as hg from 'harfang';
import { runWindow } from './js/window.js';

export function main(options = {}) {
  return runWindow('HarfangJs - Load from Assets', () => {
    // runWindow mounts resources_compiled as an assets source.
    // Load a texture from the assets system.
    // Native uint64 arguments are BigInt; multiple results form a JS array.
    const [texture, info] = hg.LoadTextureFromAssets('pictures/owl.jpg', 0n);

    if (!hg.IsValid(texture)) {
      throw Error('Failed to load pictures/owl.jpg');
    }

    console.log(`Texture dimensions: ${info.width}x${info.height}`);

    return {
      draw() {},

      // cleanup
      dispose() {
        hg.DestroyTexture(texture);
      },
    };
  }, { frameLimit: 1, ...options });
}

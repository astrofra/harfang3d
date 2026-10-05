// Port of filesystem_assets.lua / .nut. runWindow mounts resources_compiled.
import * as hg from 'harfang';
import {runWindow} from './js/window.js';

export function main(options = {}) {
  return runWindow('HarfangJs - Load from Assets', () => {
    // Native uint64 arguments are BigInt; multiple results form a JS array.
    const [texture, info] = hg.LoadTextureFromAssets('pictures/owl.jpg', 0n);
    if (!hg.IsValid(texture)) throw Error('Failed to load pictures/owl.jpg');
    console.log(`Texture dimensions: ${info.width}x${info.height}`);
    return {
      draw() {},
      dispose() { hg.DestroyTexture(texture); },
    };
  }, {frameLimit: 1, ...options});
}

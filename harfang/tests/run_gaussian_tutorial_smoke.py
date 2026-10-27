#!/usr/bin/env python3
"""Exercise the real tutorial with no arguments, using an installed Lua runtime."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

# The test controls termination through LUA_INIT, leaving the tutorial's interface unchanged.
INIT = r'''
local original_require = require
function require(name)
    local hg = original_require(name)
    if name ~= 'harfang' then return hg end
    local frames, loaded = 0, false
    local frame, is_open = hg.Frame, hg.IsWindowOpen
    local load_buffer, shutdown = hg.LoadComputeBufferFromFile, hg.RenderShutdown
    hg.LoadComputeBufferFromFile = function(path, ...)
        assert(path:match('^resources_compiled/gaussian_splatting/[^/]+/full%.bin$'), path)
        local buffer = load_buffer(path, ...)
        assert(hg.GetComputeBufferCount(buffer) == 113648 * 4)
        loaded = true
        return buffer
    end
    hg.Frame = function(...)
        frames = frames + 1
        return frame(...)
    end
    hg.IsWindowOpen = function(win)
        return frames < 120 and is_open(win)
    end
    hg.RenderShutdown = function(...)
        shutdown(...)
        assert(loaded and frames == 120, 'Tutorial did not render 120 frames of Houseplant')
        print('PASS no-argument tutorial: installed runtime, compiled Houseplant, 120 frames and normal shutdown')
    end
    return hg
end
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lua', type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix='gsplat-tutorial-smoke-') as directory:
        init = Path(directory) / 'init.lua'
        init.write_text(INIT)
        env = dict(os.environ)
        env['LUA_INIT'] = '@' + str(init)
        env['LUA_INIT_5_4'] = env['LUA_INIT']
        env['LUA_INIT_5_3'] = env['LUA_INIT']
        result = subprocess.run([str(args.lua.resolve()), 'gaussian_splatting.lua'],
                                cwd=repo / 'tutorials', env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    if result.returncode or 'PASS no-argument tutorial' not in result.stdout:
        raise RuntimeError(result.stdout)
    print(result.stdout.splitlines()[-1])


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""GPU pixel oracle: unequal/equal depths, padding and culling on the actual tutorial shaders."""
import argparse
import math
from pathlib import Path
import shutil
import struct
import subprocess
from run_gaussian_splatting import executable


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--negative-control', action='store_true', help='Disable sorting in generated shaders and require the oracle to reject it')
    parser.add_argument('--renderer', choices=['metal', 'dx11'], default='metal')
    args = parser.parse_args()
    build = args.build_dir.resolve()
    repo = Path(__file__).resolve().parents[2]
    root = build / 'gaussian_oracle' / args.renderer
    source, compiled = root / 'generated', root / 'compiled'
    source.mkdir(parents=True, exist_ok=True)
    compiled.mkdir(exist_ok=True)
    # Input order deliberately differs from depth order. Green/yellow tie in depth.
    records = [((0, 0, -.4), (1, 0, 0), .5), ((0, 0, .6), (0, 0, 1), .5),
               ((0, 0, 0), (0, 1, 0), .5), ((0, 0, 0), (1, 1, 0), .5),
               ((0, 0, -5), (1, 0, 1), 1), ((100, 0, 0), (1, 1, 1), 1),
               ((0, 0, -1), (1, 1, 1), 0)]
    header = struct.pack('<4sIII6f6I', b'HGS0', 1, len(records), 64, -1, -1, -5, 100, 1, 1, *([0]*6))
    payload = b''.join(struct.pack('<16f', *pos, opacity, .2, .2, .2, 0, 1, 0, 0, 0, *rgb, 0) for pos, rgb, opacity in records)
    (source / 'fixture.bin').write_bytes(header + payload)
    (source / 'metadata.luadata').write_text("return {version=1,presets={full={file='fixture.bin',count=7,offset=64}}}\n")
    shaders = source / 'shaders'
    shutil.copytree(repo / 'tutorials/resources/gaussian_splatting/shaders', shaders, dirs_exist_ok=True)
    for name in ['bgfx_compute.sh', 'bgfx_shader.sh']:
        shutil.copyfile(repo / 'tutorials/resources/core/shader' / name, shaders / name)
    if args.negative_control:
        p = shaders / 'sort.sc'
        p.write_text(p.read_text().replace('before == first ? a : b', 'a'))
    shaderc = executable(build / 'extern/bgfx', 'shaderc', 'Release')
    assetc = executable(build / 'tools/assetc', 'assetc', 'Release')
    lua = executable(build / 'extern/lua', 'lua', 'Release')
    module = build / 'languages/hg_lua'
    if not any((module / n).is_file() for n in ['harfang.so', 'harfang.dll']):
        module /= 'Release'

    def run(label, command):
        with (root / (label+'.log')).open('w') as log:
            result = subprocess.run([str(x) for x in command], stdout=log, stderr=subprocess.STDOUT, timeout=60)
        if result.returncode:
            raise RuntimeError(f'{label} failed: {root / (label+".log")}')

    run('assetc', [assetc, '-api', 'MTL' if args.renderer=='metal' else 'DX11', '-platform', 'osx' if args.renderer=='metal' else 'windows', '-toolchain', shaderc.parent, source, compiled])
    run('viewer', [lua, repo / 'harfang/tests/gaussian_splatting_runner.lua', '--assets', compiled, '--module', module, '--renderer', args.renderer, '--frames', '8', '--yaw', '0', '--pitch', '0', '--width', '160', '--height', '160', '--capture', root / 'oracle.png'])
    # Independent scalar covariance and alpha compositing reference at the central pixel.
    # Read the actual drawable size from the PNG using the existing Picture API.
    checker = root / 'check.lua'
    checker.write_text('''package.cpath=arg[1]..'/?.so;'..arg[1]..'/?.dll;'..package.cpath
local hg=require('harfang')
local p=hg.Picture(); assert(hg.LoadPNG(p,arg[2]))
local w,h=p:GetWidth(),p:GetHeight()
local rgb={0.025,0.03,0.04}
local order={{4,{0,0,1}},{3.4,{0,1,0}},{3.4,{1,1,0}},{3,{1,0,0}}}
for _,s in ipairs(order) do
  local variance=(h/(2*math.tan(math.pi/6))*.2/s[1])^2+.3
  local a=.5*math.exp(-.25/variance)
  for j=1,3 do rgb[j]=s[2][j]*a+rgb[j]*(1-a) end
end
local c=p:GetPixelRGBA(w//2,h//2)
for j,v in ipairs({c.r,c.g,c.b}) do assert(math.abs(v-rgb[j])<3/255,string.format('center channel %d: %.5f expected %.5f',j,v,rgb[j])) end
local corner=p:GetPixelRGBA(0,0)
assert(math.abs(corner.r-.025)<1/255 and math.abs(corner.g-.03)<1/255 and math.abs(corner.b-.04)<1/255,'Culled splats corrupted background')
print('PASS Gaussian GPU pixel oracle: depth order, ties, padding, behind-camera/offscreen/zero-opacity culling, premultiplied blending')
''')
    if args.negative_control:
        result = subprocess.run([str(x) for x in [lua, checker, module, root / 'oracle.png']], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        (root / 'negative-check.log').write_text(result.stdout)
        assert result.returncode != 0 and 'center channel' in result.stdout, 'Negative control did not fail with the expected pixel mismatch'
        print('PASS negative control: identity sorting rejected by the pixel oracle')
    else:
        run('check', [lua, checker, module, root / 'oracle.png'])
        print((root / 'check.log').read_text().strip())
    print(f'Evidence: {root}')


if __name__ == '__main__':
    main()

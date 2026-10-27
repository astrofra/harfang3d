#!/usr/bin/env python3
"""Compile the actual tutorial sorter and compare every sorted GPU ID to Python's total order."""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tutorials/tools'))
from run_gaussian_splatting import executable


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--renderer', choices=['metal', 'dx11'], default='metal')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    build = args.build_dir.resolve()
    root = build / 'gaussian_sort_probe' / args.renderer
    source, compiled = root / 'generated', root / 'compiled'
    source.mkdir(parents=True, exist_ok=True)
    compiled.mkdir(exist_ok=True)
    for p in (repo / 'harfang/tests/data/gaussian_sort_probe').iterdir():
        shutil.copyfile(p, source / p.name)
    shutil.copyfile(repo / 'tutorials/resources/gaussian_splatting/shaders/sort.sc', source / 'sort.sc')
    for name in ['bgfx_shader.sh', 'bgfx_compute.sh']:
        shutil.copyfile(repo / 'tutorials/resources/core/shader' / name, source / name)
    depths = [float((i*73)%4096) if i<113648 and i%7 else -1.0 for i in range(131072)]
    (source / 'input.bin').write_bytes(b''.join(struct.pack('<4f', depth, i, 0, 0) for i,depth in enumerate(depths)))
    order = sorted(range(len(depths)), key=lambda i: (-depths[i], i))
    (source / 'expected.bin').write_bytes(struct.pack('<131072I', *order))
    assetc = executable(build / 'tools/assetc', 'assetc', 'Release')
    shaderc = executable(build / 'extern/bgfx', 'shaderc', 'Release')
    lua = executable(build / 'extern/lua', 'lua', 'Release')
    module = build / 'languages/hg_lua'
    if not any((module / n).is_file() for n in ['harfang.so','harfang.dll']):
        module /= 'Release'
    commands = [('assetc', [assetc,'-api','MTL' if args.renderer=='metal' else 'DX11','-platform','osx' if args.renderer=='metal' else 'windows','-toolchain',shaderc.parent,source,compiled]),
                ('sort', [lua,repo / 'harfang/tests/gaussian_sort_probe.lua',compiled,module,args.renderer])]
    for label,command in commands:
        with (root / (label+'.log')).open('w') as log:
            result = subprocess.run([str(x) for x in command], stdout=log, stderr=subprocess.STDOUT, timeout=60)
        if result.returncode:
            raise RuntimeError(f'{label} failed: {root / (label+".log")}')
    assert 'PASS GPU sort' in (root / 'sort.log').read_text()
    print((root / 'sort.log').read_text().splitlines()[-1])
    print(f'Evidence: {root}')


if __name__ == '__main__':
    main()

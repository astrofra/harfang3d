#!/usr/bin/env python3
"""Compile the reference core-package sorter and compare every sorted GPU ID to Python's total order."""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess


def executable(directory, name, configuration):
    for folder in (directory / configuration, directory):
        for suffix in ('.exe', ''):
            path = folder / (name + suffix)
            if path.is_file():
                return path
    raise FileNotFoundError(f'{name} under {directory}')


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
    (source / 'core/gaussian_splatting').mkdir(parents=True, exist_ok=True)
    (source / 'core/shader').mkdir(parents=True, exist_ok=True)
    for name in ['sort.sc', 'sort_local.sc', 'radix_histogram.sc', 'radix_prefix.sc', 'radix_scatter.sc', 'gather.sc']:
        shutil.copyfile(repo / 'core-package/core/gaussian_splatting' / name, source / 'core/gaussian_splatting' / name)
    for name in ['bgfx_shader.sh', 'bgfx_compute.sh']:
        shutil.copyfile(repo / 'core-package/core/shader' / name, source / name)
        shutil.copyfile(repo / 'core-package/core/shader' / name, source / 'core/shader' / name)
    cases = [(n, 'mixed') for n in (1, 2, 128, 256, 512, 1024, 131072, 2097152)]
    cases += [(1024, 'ties'), (1024, 'invisible')]
    manifest = []
    for n, pattern in cases:
        name = f'{n}_{pattern}'
        actual = max(1, n * 113648 // 131072)
        # Adjacent float32 mantissas exercise all key bits, plus ties, culling and padding.
        depths = [struct.unpack('<f', struct.pack('<I', 0x3f000000 | ((i * 2654435761) & 0x7fffff)))[0]
                  if i % 11 else 1.0 for i in range(n)]
        if pattern == 'ties':
            depths = [1.0] * n
        depths = [d if i < actual and (i % 7 or n < 8) and pattern != 'invisible' else -1.0
                  for i, d in enumerate(depths)]
        (source / f'{name}_input.bin').write_bytes(b''.join(struct.pack('<4f', d, i, 0, 0) for i, d in enumerate(depths)))
        order = sorted(range(n), key=lambda i: (-depths[i], i))
        (source / f'{name}_expected.bin').write_bytes(b''.join(
            struct.pack('<II', i, int(slot < actual and depths[i] > 0)) for slot, i in enumerate(order)))
        (source / f'{name}_projected.bin').write_bytes(b''.join(
            struct.pack('<12f', (i & 255)/255, ((i >> 8) & 255)/255, ((i >> 16) & 255)/255, 1,
                        0, 0, 0, 0, 0, 0, 0, 0) for i in range(n)))
        manifest.append(f'{name} {n} {actual}')
    (source / 'cases.txt').write_text('\n'.join(manifest) + '\n')
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
            result = subprocess.run([str(x) for x in command], stdout=log, stderr=subprocess.STDOUT, timeout=240)
        if result.returncode:
            raise RuntimeError(f'{label} failed: {root / (label+".log")}')
    assert 'PASS GPU sort' in (root / 'sort.log').read_text()
    print((root / 'sort.log').read_text().splitlines()[-1])
    print(f'Evidence: {root}')


if __name__ == '__main__':
    main()

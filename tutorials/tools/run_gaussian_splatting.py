#!/usr/bin/env python3
"""Reproducibly prepare, compile and optionally run the Lua Gaussian tutorial."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
from prepare_gaussian_splat import convert

SOURCE_SHA256 = 'ce9d490b18f16d6730d832227bf541431b8b1a5d167c5a3fe89af4f324c0d8fd'


def executable(directory, name, config):
    name += '.exe' if sys.platform == 'win32' else ''
    for p in [directory / name, directory / config / name]:
        if p.is_file():
            return p
    raise RuntimeError(f'Missing executable: {directory / name}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--renderer', choices=['metal', 'dx11'], default='metal')
    parser.add_argument('--config', default='Release')
    parser.add_argument('--build', action='store_true', help='Build engine/tools in the existing CMake configuration')
    parser.add_argument('--compile-only', action='store_true')
    parser.add_argument('--preset', choices=['full', '60k', '30k'], default='full')
    parser.add_argument('--frames', type=int, default=0, help='0: interactive; positive: automatically stop')
    parser.add_argument('--resize-test', action='store_true', help='Exercise two window size changes; use at least 90 frames')
    parser.add_argument('--route', action='store_true', help='Repeatable full orbit for qualification')
    parser.add_argument('--capture', action='store_true', help='Save last frame; requires --frames')
    parser.add_argument('--width', type=int, default=1280, help='Window client width; drawable may differ on Retina')
    parser.add_argument('--height', type=int, default=720)
    args = parser.parse_args()
    if args.frames < 0 or (args.capture and args.frames == 0) or min(args.width, args.height) <= 0:
        parser.error('Positive sizes and nonnegative frames required; capture needs bounded frames')
    repo = Path(__file__).resolve().parents[2]
    build = args.build_dir.resolve()
    if not (build / 'CMakeCache.txt').is_file():
        parser.error('--build-dir must be a configured HARFANG build (ASSETC and HG_LUA enabled)')
    root = build / 'gaussian_splatting' / args.renderer
    source, compiled = root / 'generated', root / 'compiled'
    source.mkdir(parents=True, exist_ok=True)
    compiled.mkdir(exist_ok=True)

    def run(label, command, timeout):
        print(f'{label}: log at {root / (label + ".log")}', flush=True)
        with (root / (label + '.log')).open('w') as log:
            result = subprocess.run([str(x) for x in command], stdout=log, stderr=subprocess.STDOUT, cwd=repo, timeout=timeout)
        if result.returncode:
            raise RuntimeError(f'{label} failed ({result.returncode}); see {root / (label + ".log")}')

    if args.build:
        run('build', ['cmake', '--build', build, '--config', args.config, '--target', 'hg_lua', 'lua', 'assetc', 'shaderc', '--parallel', '4'], 900)
    manifest = convert(repo / 'tutorials/resources/gaussian_splatting/source/plant.splat', source, SOURCE_SHA256)
    assert manifest['source_count'] == 113648
    # Keep data as portable Lua source, propagated by assetc's copy rule.
    (source / 'metadata.lua').replace(source / 'metadata.luadata')
    shaders = source / 'shaders'
    shaders.mkdir(exist_ok=True)
    for p in (repo / 'tutorials/resources/gaussian_splatting/shaders').iterdir():
        if p.is_file():
            shutil.copyfile(p, shaders / p.name)
    for name in ['bgfx_shader.sh', 'bgfx_compute.sh']:
        shutil.copyfile(repo / 'tutorials/resources/core/shader' / name, shaders / name)
    shutil.copyfile(repo / 'tutorials/resources/gaussian_splatting/ATTRIBUTION.md', source / 'ATTRIBUTION.md')
    assetc = executable(build / 'tools/assetc', 'assetc', args.config)
    shaderc = executable(build / 'extern/bgfx', 'shaderc', args.config)
    run('assetc', [assetc, '-api', 'MTL' if args.renderer == 'metal' else 'DX11', '-platform', 'osx' if args.renderer == 'metal' else 'windows', '-toolchain', shaderc.parent, source, compiled], 180)
    for name in ['project.sc', 'sort.sc', 'gather.sc', 'splat.vsb', 'splat.fsb', 'gizmo.vsb', 'gizmo.fsb']:
        p = compiled / 'shaders' / name
        if not p.is_file() or p.stat().st_size == 0:
            raise RuntimeError(f'Missing compiled shader {p}; see assetc.log')
    for name in ['metadata.luadata', 'full.bin', '60k.bin', '30k.bin', 'ATTRIBUTION.md']:
        if not (compiled / name).is_file():
            raise RuntimeError(f'assetc did not propagate {name}')
    # Publish assetc's output beside the other tutorials' compiled resources.
    tutorial_assets = repo / 'tutorials/resources_compiled/gaussian_splatting' / args.renderer
    shutil.copytree(compiled, tutorial_assets, dirs_exist_ok=True)
    print(f'Tutorial resources: {tutorial_assets}', flush=True)
    if args.compile_only:
        print(f'Compiled only, runtime unverified: {compiled}')
        return
    lua = executable(build / 'extern/lua', 'lua', args.config)
    module = build / 'languages/hg_lua'
    if not any((module / n).is_file() for n in ['harfang.so', 'harfang.dll']):
        module /= args.config
    command = [lua, repo / 'harfang/tests/gaussian_splatting_runner.lua', '--assets', compiled, '--module', module,
               '--renderer', args.renderer, '--preset', args.preset, '--frames', str(args.frames),
               '--width', str(args.width), '--height', str(args.height), '--report', root / f'{args.preset}-report.json']
    if args.route:
        command.append('--route')
    if args.resize_test:
        command.append('--resize-test')
    if args.capture:
        command += ['--capture', root / f'{args.preset}.png']
    run('viewer', command, max(60, args.frames / 5) if args.frames else None)
    print((root / f'{args.preset}-report.json').read_text())
    print(f'Evidence: {root}')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        sys.exit(str(error))

"""Prepare isolated native tutorial test fixtures; not needed to run with Lua assets."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
TUTORIALS = (
    'draw_lines', 'draw_model_no_pipeline', 'filesystem_assets',
    'game_mouse_flight', 'scene_many_nodes', 'scene_pbr',
)


def stage_tutorials(source):
    source.mkdir(parents=True, exist_ok=True)
    for name in TUTORIALS:
        shutil.copy2(ROOT / 'tutorials' / (name + '.js'), source / (name + '.js'))
    (source / 'js').mkdir(exist_ok=True)
    shutil.copy2(ROOT / 'tutorials/js/window.js', source / 'js/window.js')
    (source / 'shaders').mkdir(exist_ok=True)
    shaders = ROOT / 'tutorials/resources/shaders'
    for pattern in ['white_*', 'mdl_*', 'pos_rgb_*', 'bgfx_shader.sh']:
        for path in shaders.glob(pattern):
            shutil.copy2(path, source / 'shaders' / path.name)
    (source / 'pictures').mkdir(exist_ok=True)
    shutil.copy2(ROOT / 'tutorials/resources/pictures/owl.jpg', source / 'pictures/owl.jpg')

    # Scene tutorials share the original Lua models, materials and lighting probe.
    resources = ROOT / 'tutorials/resources'
    for name in ['playground', 'paper_plane', 'materials']:
        shutil.copytree(resources / name, source / name, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('*.editor', 'fbx_importer_cfg.txt'))

    (source / 'core/shader').mkdir(parents=True, exist_ok=True)
    for pattern in ['default*', 'pbr*', 'forward_pipeline.sh', 'bgfx_shader.sh']:
        for path in (resources / 'core/shader').glob(pattern):
            shutil.copy2(path, source / 'core/shader' / path.name)

    (source / 'core/pbr').mkdir(parents=True, exist_ok=True)
    for name in ['brdf.dds', 'probe.hdr', 'probe.hdr.meta']:
        shutil.copy2(resources / 'core/pbr' / name, source / 'core/pbr' / name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assetc', type=Path, default=ROOT.parent / 'install/assetc/assetc.exe')
    parser.add_argument('--source', type=Path, default=ROOT.parent / 'build/hgjs-tutorials/source')
    parser.add_argument('--output', type=Path, default=ROOT.parent / 'build/hgjs-tutorials/resources_compiled')
    parser.add_argument('--api', choices=['DX11', 'DX12', 'GL', 'GLES', 'VK', 'MTL'], help='Override assetc\'s native graphics API default; the application renderer must match')
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    if source == output or source.is_relative_to(output) or output.is_relative_to(source):
        parser.error('Source and compiled assets must be separate directories')
    stage_tutorials(source)
    output.mkdir(parents=True, exist_ok=True)
    command = [str(args.assetc.resolve())]
    if args.api:
        command += ['-api', args.api]
    run = subprocess.run([*command, str(source), str(output)], capture_output=True, text=True)
    log = output.parent / 'tutorials-assetc.log'
    log.write_text(run.stdout + run.stderr, encoding='utf-8')
    if run.returncode or re.search(r'\b[1-9][0-9]* failed\b|FAILED:', run.stdout + run.stderr):
        raise RuntimeError(f'assetc failed: see {log}')
    print(f'Compiled tutorials: {output}')


if __name__ == '__main__':
    main()

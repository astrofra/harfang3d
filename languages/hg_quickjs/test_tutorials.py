"""Validate the additional QuickJS tutorials using the existing Lua assets."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

from prepare_tutorials import CONSOLE_TUTORIALS, PHYSICS_TUTORIALS

ROOT = Path(__file__).resolve().parents[2]
TUTORIALS = (
    'draw_lines_starfield', 'draw_text', 'imgui_basic',
    'scene_draw_to_multiple_viewports', 'physics_impulse',
    'draw_and_create_model_no_pipeline', 'material_update_value',
    'scene_draw_to_texture', 'scene_instances', 'scene_lua_script',
    'draw_text_over_models', 'imgui_edit', 'scene_light_priority',
    'model_builder', 'physics_manual_setup',
    'imgui_mouse_capture', 'render_resize_to_window', 'input_list_devices',
    'physics_overrides_matrix', 'scene_spot_shadow_clip',
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('hgjs', type=Path)
    parser.add_argument('--tutorials', type=Path, default=ROOT / 'tutorials',
                        help='Directory containing the JavaScript entries')
    parser.add_argument('--cwd', type=Path, default=ROOT / 'tutorials',
                        help='Directory containing resources_compiled')
    parser.add_argument('--output', type=Path, default=ROOT.parent / 'build/hgjs-tutorials/additional-validation')
    parser.add_argument('--renderer', choices=['default', 'GL'], default='default')
    parser.add_argument('--frames', type=int, default=120)
    parser.add_argument('--skip-physics', action='store_true', help='For builds without Bullet')
    parser.add_argument('--only', choices=TUTORIALS, nargs='+', help='Run only these tutorials')
    args = parser.parse_args()
    if args.frames < 6:
        parser.error('--frames must be at least 6 to flush the screenshot')

    executable = args.hgjs.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    selected = args.only or TUTORIALS
    cases = [(name, {}) for name in selected if not (args.skip_physics and name in PHYSICS_TUTORIALS)]
    if not args.skip_physics and 'physics_impulse' in selected:
        cases.append(('physics_impulse', {'useForce': False}))

    with tempfile.TemporaryDirectory(prefix='hgjs-tutorials-') as temporary:
        entry = Path(temporary) / 'smoke.js'
        for name, extra in cases:
            label = name + ('-impulse' if extra else '')
            # A fresh directory prevents old captures from satisfying the check.
            with tempfile.TemporaryDirectory(prefix=label + '-', dir=output) as capture_dir:
                capture = Path(capture_dir) / 'frame'
                options = dict(hidden=True, frameLimit=args.frames, capturePath=capture.as_posix(),
                               captureFrame=args.frames - 3, **extra)
                module = (args.tutorials.resolve() / (name + '.js')).as_posix()
                screenshot_check = (
                    "  const picture = new hg.Picture();\n"
                    f"  if (!hg.LoadPicture(picture, {json.dumps(capture.as_posix() + '.tga')}) ||\n"
                    f"      !hg.SavePNG(picture, {json.dumps((output / (label + '.png')).as_posix())}))\n"
                    "    throw Error('Missing tutorial capture');\n"
                ) if name not in CONSOLE_TUTORIALS else ''
                entry.write_text(
                    "import * as hg from 'harfang';\n"
                    f"import {{main as run}} from {json.dumps(module)};\n"
                    "export async function main() {\n"
                    f"  const options = {json.dumps(options)};\n"
                    + ("  options.renderer = hg.RT_OpenGL;\n" if args.renderer == 'GL' else '')
                    + "  await run(options);\n"
                    + screenshot_check
                    + f"  console.log('TUTORIAL_OK {label}');\n"
                    "}\n", encoding='utf-8')
                run = subprocess.run([str(executable), str(entry)], cwd=args.cwd.resolve(),
                                     capture_output=True, text=True, timeout=90)
                log = output / (label + '.log')
                log.write_text(run.stdout + run.stderr, encoding='utf-8')
                if run.returncode or f'TUTORIAL_OK {label}' not in run.stdout:
                    raise RuntimeError(f'{label} failed (exit {run.returncode}): see {log}')
                print(f'TUTORIAL_OK {label}', flush=True)
    print(f'Logs and screenshots: {output}')


if __name__ == '__main__':
    main()

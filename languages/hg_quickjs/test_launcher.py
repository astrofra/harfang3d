"""Check the native JS file CLI independently of the asset compiler and renderer."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile


def check_launcher(hgjs):
    hgjs = str(Path(hgjs).resolve())
    with tempfile.TemporaryDirectory(prefix='hgjs file cli ') as temporary:
        root = Path(temporary)
        app = root / "app's scripts"
        (app / 'sub').mkdir(parents=True)
        (root / 'resources_compiled').mkdir()
        (root / 'resources_compiled/probe.txt').write_text('compiled resource')
        (root / 'cwd.txt').write_text('caller cwd')
        (app / 'cwd.txt').write_text('wrong cwd')
        (app / 'shared.js').write_text('let count = 0; export function next() { return ++count; }')
        (app / 'sub/importer.js').write_text("export {next} from '../shared.js';")
        (app / 'sub/main.js').write_text('''import * as hg from 'harfang';
import {nextFrame} from 'harfang-host';
import {next} from './importer.js';
import {next as same} from '../sub/../shared.js';
export async function main() {
    if (next() !== 1 || same() !== 2) throw Error('Module cache/parent imports');
    const dynamic = await import('../shared.js');
    if (dynamic.next() !== 3) throw Error('Dynamic import cache');
    if (hg.FileToString('cwd.txt') !== 'caller cwd') throw Error('Launcher changed cwd');
    if (hg.IsAssetFile('cwd.txt') || hg.IsAssetFile('shared.js')) throw Error('Implicit asset mount');
    if (!hg.AddAssetsFolder('resources_compiled') || !hg.IsAssetFile('probe.txt')) throw Error('Explicit asset mount');
    if (new hg.Vec3(2).add(new hg.Vec3(3)).x !== 5) throw Error('Native binding');
    if (typeof (await nextFrame()).dtNs !== 'bigint') throw Error('Host module');
    console.log('CLI_OK ' + JSON.stringify(scriptArgs.slice(1)));
}
''')

        def run(*args, code=0, contains=None):
            result = subprocess.run([hgjs, *map(str, args)], cwd=root, capture_output=True, text=True, timeout=15)
            assert result.returncode == code, (args, result.returncode, result.stdout, result.stderr)
            if contains:
                assert contains in result.stdout + result.stderr, (args, result.stdout, result.stderr)
            return result

        arguments = ['hello world', 'say "hi"', '--application-option']
        for entry in [str(app / 'sub/main.js'), os.path.relpath(app / 'sub/main.js', root)]:
            result = run(entry, *arguments, contains='CLI_OK ')
            line = next(line for line in result.stdout.splitlines() if line.startswith('CLI_OK '))
            assert json.loads(line.removeprefix('CLI_OK ')) == arguments

        # Ordinary top-level scripts do not need main() or completion exports.
        (app / 'plain.js').write_text("import * as hg from 'harfang'; console.log('PLAIN_OK', new hg.Vec3(7).x);")
        run(app / 'plain.js', contains='PLAIN_OK 7')
        (app / 'native.js').write_text('''
const deadline = Date.now() + 5200;
while (Date.now() < deadline) {} // Native synchronous loops have no 5 second deadline.
const buffer = new Uint8Array(260 * 1024 * 1024); // No prototype 256 MiB heap cap.
buffer[buffer.length - 1] = 42;
if (buffer[buffer.length - 1] !== 42) throw Error('Native heap allocation');
print('NATIVE_LOOP_OK'); console.error('STDERR_OK');
''')
        result = run(app / 'native.js', contains='NATIVE_LOOP_OK')
        assert 'STDERR_OK' in result.stderr and 'STDERR_OK' not in result.stdout
        (app / 'detached.js').write_text('''
import {nextFrame} from 'harfang-host';
export function main() {
    (async () => { await nextFrame(); await nextFrame(); print('DETACHED_OK'); })();
}
''')
        run(app / 'detached.js', contains='DETACHED_OK')
        (app / 'broken.js').write_text("import './missing.js';")
        run(app / 'broken.js', code=1, contains='missing.js')
        (app / 'reject.js').write_text("export const completion = Promise.reject(Error('FILE_REJECTION'));")
        run(app / 'reject.js', code=1, contains='FILE_REJECTION')
        run('missing-entry.js', code=1, contains='missing-entry.js')
        run(code=2, contains='Usage: hgjs <script.js>')
        help_result = run('--help', contains='Usage: hgjs <script.js>')
        assert '--assets' not in help_result.stdout
        run('-h', contains='Usage:')
        run('-v', contains='QuickJS')
        run('--version', contains='HARFANG')
        run('--assets', code=2, contains='Usage:')
        # The removed option must also fail when supplied with its former operands.
        run('--assets', root / 'resources_compiled', app / 'plain.js', code=2, contains='Usage:')
        run(Path(__file__).with_name('test_native_api.js').resolve(), contains='HGJS_NATIVE_API_OK')
    print('HGJS file CLI passed: paths, imports/cache, args, explicit assets, synchronous loops, heap, async frames, output streams and errors')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('hgjs', type=Path)
    check_launcher(parser.parse_args().hgjs)

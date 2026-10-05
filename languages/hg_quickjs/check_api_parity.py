"""Compare the declared native API across FABGen's Lua, Squirrel and JS backends."""
import argparse
import contextlib
import io
import json
from pathlib import Path
import sys
import subprocess
import tempfile


def manifest(generator):
    api = set()
    for item in generator._bound_functions:
        api.add('function:' + str(item['bound_name']))
    for item in generator._bound_variables:
        api.add('constant:' + str(item['bound_name']))
    for enum in generator._enums.values():
        api.update('constant:' + str(name) for name in enum)
    for conv in generator._bound_types:
        if not conv.is_type_class() or conv.nobind:
            continue
        name = str(conv.bound_name)
        api.add('class:' + name)
        if conv.constructor:
            api.add('constructor:' + name)
        for group in ['methods', 'static_methods', 'members', 'static_members']:
            for member in getattr(conv, group):
                api.add(group + ':' + name + '.' + str(member.get('bound_name', member['name'])))
        for group in ['arithmetic_ops', 'comparison_ops']:
            for op in getattr(conv, group):
                api.add('operator:' + name + '.' + op['op'])
    return api


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fabgen', type=Path, default=root.parent / 'FABGen')
    parser.add_argument('--defines', default='HG_ENABLE_BULLET3_SCENE_PHYSICS,HG_ENABLE_RECAST_DETOUR_API')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--hgjs', type=Path, help='Also check that the compared API is exported by this executable')
    args = parser.parse_args()
    sys.path[:0] = [str(args.fabgen.resolve()), str(root / 'binding')]
    import gen
    import lang.lua
    import lang.squirrel
    import lang.quickjs
    import bind_harfang

    manifests = {}
    for backend in [lang.lua.LuaGenerator, lang.squirrel.SquirrelGenerator, lang.quickjs.QuickJSGenerator]:
        with contextlib.redirect_stdout(io.StringIO()):
            generator = backend()
            gen.api_prefix = 'hg_' + generator.get_language().lower()
            generator.defines = args.defines.split(',')
            generator.embedded = True
            bind_harfang.bind(generator)
        manifests[generator.get_language()] = manifest(generator)
    # Lua values are passed directly; external languages expose LuaObject wrappers.
    differences = {name: sorted(api - manifests['QuickJS']) for name, api in manifests.items() if name != 'QuickJS'}
    report = dict(defines=generator.defines, counts={name: len(api) for name, api in manifests.items()},
                  missingInQuickJS=differences,
                  scope='Declared classes, constructors, functions, constants, methods, members and operators. Not behavioral equivalence or an overload/conversion audit.')
    if args.hgjs:
        # Use the union of the reference bindings, not JS's own declarations.
        reference = sorted(manifests['Lua'] | manifests['Squirrel'])
        source = ("import * as hg from 'harfang';\nconst operators = " + json.dumps(lang.quickjs.OPERATORS)
                  + ';\nconst api = ' + json.dumps(reference) + ';\n') + '''
for (const entry of api) {
    const [kind, path] = entry.split(':'), [name, member] = path.split('.');
    const value = hg[name];
    let valid = value !== undefined;
    if (kind === 'function' || kind === 'constructor') valid = typeof value === 'function';
    if (kind === 'class') valid = typeof value === 'function' && !!value.prototype;
    if (member) {
        const target = kind.startsWith('static_') ? value : value?.prototype;
        const key = kind === 'operator' ? operators[member] : member;
        const descriptor = target && Object.getOwnPropertyDescriptor(target, key);
        valid = !!descriptor;
        if (kind.endsWith('methods') || kind === 'operator') valid = typeof descriptor?.value === 'function';
    }
    if (!valid) throw Error('Missing runtime API: ' + entry);
}
print('HGJS_API_SURFACE_OK');
'''
        with tempfile.TemporaryDirectory(prefix='hgjs api parity ') as temporary:
            entry = Path(temporary) / 'check.js'
            entry.write_text(source, encoding='utf-8')
            result = subprocess.run([str(args.hgjs.resolve()), str(entry)], capture_output=True, text=True, timeout=30)
            if result.returncode or 'HGJS_API_SURFACE_OK' not in result.stdout:
                raise RuntimeError(result.stdout + result.stderr)
        report['runtimeSurface'] = dict(status='pass', referenceEntries=len(reference))
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
    return int(any(differences.values()))


if __name__ == '__main__':
    sys.exit(main())

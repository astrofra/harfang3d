#!/usr/bin/env python3
"""Compare imported animation playback with an independent glTF evaluator."""
import argparse
import bisect
import copy
import json
import math
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

from audit_importer import Fixture, fixtures
from compare_exports import load, accessor


def normalize(q):
    length = math.sqrt(sum(x * x for x in q))
    return [x / length for x in q]


def evaluate(times, values, interpolation, rotation, time):
    stamps = [int(t * 1e9) for t in times]
    cubic = interpolation == 'CUBICSPLINE'
    key = lambda i: values[3 * i + 1 if cubic else i]
    upper = bisect.bisect_right(stamps, time)
    if upper == 0:
        value = key(0)
    elif upper == len(times):
        value = key(-1) if not cubic else values[-2]
    else:
        i = upper - 1
        u = (time - stamps[i]) / (stamps[upper] - stamps[i])
        a, b = key(i), key(upper)
        if interpolation == 'STEP':
            value = a
        elif cubic:
            dt = times[upper] - times[i]
            value = [(2*u**3-3*u*u+1)*x + (u**3-2*u*u+u)*dt*tx +
                     (-2*u**3+3*u*u)*y + (u**3-u*u)*dt*ty
                     for x, tx, y, ty in zip(a, values[3*i+2], b, values[3*upper])]
        elif rotation:
            a, b = normalize(a), normalize(b)
            dot = sum(x*y for x, y in zip(a, b))
            if dot < 0:
                b, dot = [-x for x in b], -dot
            angle = math.acos(min(dot, 1))
            value = [x*(1-u)+y*u for x, y in zip(a, b)] if angle < 1e-6 else [
                (x*math.sin((1-u)*angle)+y*math.sin(u*angle))/math.sin(angle) for x, y in zip(a, b)]
        else:
            value = [x*(1-u)+y*u for x, y in zip(a, b)]
    return normalize(value) if rotation else list(value)


class AnimationTests(unittest.TestCase):
    def run_fixture(self, f, suffix='', invalid=False, binary=False):
        folder = self.output / (self._testMethodName + suffix)
        folder.mkdir()
        source = folder / 'input.gltf'
        f.save(source)
        model, buffers = load(source)
        if binary:
            doc = copy.deepcopy(model)
            del doc['buffers'][0]['uri']
            js = json.dumps(doc).encode()
            js += b' ' * (-len(js) % 4)
            data = buffers[0] + b'\0' * (-len(buffers[0]) % 4)
            source = folder / 'input.glb'
            source.write_bytes(struct.pack('<III', 0x46546c67, 2, 28+len(js)+len(data)) +
                               struct.pack('<II', len(js), 0x4e4f534a) + js + struct.pack('<II', len(data), 0x004e4942) + data)
        out = folder / 'out'
        run = subprocess.run([str(self.importer), str(source), '-out', str(out), '-base-resource-path', str(out)],
                             capture_output=True, text=True, timeout=30)
        (folder / 'import.log').write_text(run.stdout + run.stderr)
        if invalid:
            self.assertEqual(run.returncode, 1, run.stdout + run.stderr)
            self.assertIn('animation', run.stdout.lower())
            self.assertFalse(list(out.glob('*.scn')))
            return
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        native = json.loads((out / 'input.scn').read_text())
        clips = model.get('animations', [])
        requests = {}
        for clip in clips:
            times = sorted({int(row[0]*1e9) for s in clip['samplers'] for row in accessor(model, buffers, s['input'])})
            probes = {times[0]-1, times[-1]+1, *times}
            probes.update(t+d for t in times for d in (-1, 1))
            probes.update(times[0]+(times[-1]-times[0])*i//137 for i in range(138))
            requests[clip['name']] = sorted(probes)
        (folder / 'requests.json').write_text(json.dumps(requests))
        sample = subprocess.run([str(self.sampler), str(out/'input.scn'), str(folder/'requests.json'), str(folder/'samples.json')],
                                capture_output=True, text=True, timeout=30)
        self.assertEqual(sample.returncode, 0, sample.stdout + sample.stderr)
        actual = json.loads((folder/'samples.json').read_text())
        compiled = subprocess.run([str(self.sampler), str(out/'input.scn'), str(folder/'requests.json'),
                                   str(folder/'binary-samples.json'), '--binary-roundtrip'], capture_output=True, text=True, timeout=30)
        self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
        self.assertEqual(actual, json.loads((folder/'binary-samples.json').read_text()))
        exported = subprocess.run([str(self.exporter), '--animation-export', str(out/'input.scn'), str(folder/'roundtrip.gltf')],
                                  capture_output=True, text=True, timeout=30)
        self.assertEqual(exported.returncode, 0, exported.stdout + exported.stderr)
        roundtrip, roundtrip_buffers = load(folder/'roundtrip.gltf')
        self.assertEqual([c['name'] for c in native['scene_anims']], [c['name'] for c in clips])
        self.assertEqual([c['name'] for c in roundtrip['animations']], [c['name'] for c in clips])
        max_error = 0
        for clip in clips:
            exported_clip = next(c for c in roundtrip['animations'] if c['name'] == clip['name'])
            self.assertEqual(len(exported_clip['channels']), len(clip['channels']))
            for channel in clip['channels']:
                path = channel['target']['path']
                sampler = clip['samplers'][channel['sampler']]
                times = [row[0] for row in accessor(model, buffers, sampler['input'])]
                values = accessor(model, buffers, sampler['output'])
                name = model['nodes'][channel['target']['node']]['name']
                exported_channel = next(c for c in exported_clip['channels'] if c['target']['path'] == path and
                                        roundtrip['nodes'][c['target']['node']]['name'] == name)
                exported_sampler = exported_clip['samplers'][exported_channel['sampler']]
                mode = sampler.get('interpolation', 'LINEAR')
                self.assertEqual(exported_sampler['interpolation'], 'STEP' if mode == 'STEP' else 'LINEAR')
                exported_times = [row[0] for row in accessor(roundtrip, roundtrip_buffers, exported_sampler['input'])]
                exported_values = accessor(roundtrip, roundtrip_buffers, exported_sampler['output'])
                if mode != 'CUBICSPLINE':
                    self.assertEqual(len(exported_times), len(times))
                for sample in actual[clip['name']]:
                    expected = evaluate(times, values, sampler.get('interpolation', 'LINEAR'), path == 'rotation', sample['time'])
                    # Sub-nanosecond float->native quantization is tested separately;
                    # normal-duration fixtures also verify the re-exported curve.
                    if 'nanosecond' not in self._testMethodName:
                        replay = evaluate(exported_times, exported_values, exported_sampler['interpolation'], path == 'rotation', sample['time'])
                        error = min(math.dist(replay, expected), math.dist(replay, [-v for v in expected])) if path == 'rotation' else max(abs(x-y) for x, y in zip(replay, expected))
                        self.assertLess(error, 4e-5 + 2e-6*max(map(abs, expected)))
                    if path == 'translation': expected[2] *= -1
                    elif path == 'rotation': expected[0] *= -1; expected[1] *= -1
                    value = sample['nodes'][name][{'translation': 'position', 'rotation': 'rotation', 'scale': 'scale'}[path]]
                    if path == 'rotation':
                        error = min(math.dist(value, expected), math.dist(value, [-v for v in expected]))
                        self.assertLess(error, 2e-5, (clip['name'], path, sample['time'], error))
                    else:
                        error = max(abs(x-y) for x, y in zip(value, expected))
                        self.assertLessEqual(error, 3e-5 + 2e-6*max(map(abs, expected)), (clip['name'], path, sample['time'], value, expected))
                    max_error = max(error, max_error)
        self.metrics[folder.name] = {'max_component_or_quaternion_chord_error': max_error, 'samples': sum(map(len, actual.values())),
                                     'native_keys': sum(len(t['keys']) for a in native['anims'] for kind in ('vec3', 'quat') for t in a['anim'].get(kind, []))}
        return native

    def custom(self, path, mode, times, values):
        f = Fixture()
        input_id = f.add('SCALAR', [(t,) for t in times], bounds=True)
        output_id = f.add('VEC4' if path == 'rotation' else 'VEC3', values)
        f.model['animations'] = [{'name': 'clip', 'samplers': [{'input': input_id, 'output': output_id, 'interpolation': mode}],
                                  'channels': [{'sampler': 0, 'target': {'node': 0, 'path': path}}]}]
        return f

    def test_reordered_and_shared_samplers(self):
        for name in ('reordered_channels', 'shared_sampler'):
            f = next(f for n, f in fixtures() if n == name)
            native = self.run_fixture(f, name)
            targets = [t['target'] for a in native['anims'] for t in a['anim'].get('vec3', [])]
            self.assertCountEqual(targets, ['Position', 'Scale'])

    def test_shared_sampler_multiple_nodes_and_unused_sampler(self):
        f = self.custom('rotation', 'LINEAR', [0, 1], [(0, 0, 0, 1), (0, 0, 1, 0)])
        f.model['nodes'].append({'name': 'Other'})
        f.model['scenes'][0]['nodes'].append(1)
        a = f.model['animations'][0]
        a['channels'].append({'sampler': 0, 'target': {'node': 1, 'path': 'rotation'}})
        a['samplers'].extend([copy.deepcopy(a['samplers'][0])] * 3)
        self.run_fixture(f)

    def test_linear_nonuniform_vec3(self):
        for path in ('translation', 'scale'):
            self.run_fixture(self.custom(path, 'LINEAR', [0.125, 0.5, 2], [(2, 3, 4), (4, 1, 2), (1, 6, 3)]), path)

    def test_step_all_trs(self):
        for path in ('translation', 'scale', 'rotation'):
            values = [(0, 0, 0, 1), (0, 0, 1, 0), (0, 0, 0, -1)] if path == 'rotation' else [(1, 2, 3), (4, 5, 6), (7, 8, 9)]
            self.run_fixture(self.custom(path, 'STEP', [0.125, 0.5, 2], values), path)

    def test_spherical_rotation_and_antipodal_keys(self):
        for sign in (1, -1):
            self.run_fixture(self.custom('rotation', 'LINEAR', [0, 1, 2], [(0, 0, 0, 1), (0, 0, sign, 0), (0, 0, -sign, 0)]), str(sign))

    def test_cubic_vectors_tangent_duration_and_overshoot(self):
        for path in ('translation', 'scale'):
            values = [(0, 0, 0), (1, 2, 3), (7, -2, 1), (-3, 2, 0), (2, 1, 4), (0, 5, 2), (1, -6, 3), (3, 4, 2), (0, 0, 0)]
            self.run_fixture(self.custom(path, 'CUBICSPLINE', [0.125, 0.75, 2.5], values), path)

    def test_cubic_rotation_signs_and_glb(self):
        values = [(0, 0, 0, 0), (0, 0, 0, 1), (0.5, 0, 2, 0), (0, 0.5, -1, 0), (0, 0, -1, 0), (0, 0, 0, 0)]
        self.run_fixture(self.custom('rotation', 'CUBICSPLINE', [0, 2], values), binary=True)

    def test_long_constant_cubic_does_not_expand(self):
        values = [(0, 0, 0), (1, 2, 3), (0, 0, 0)] * 2
        native = self.run_fixture(self.custom('translation', 'CUBICSPLINE', [0, 10000], values))
        self.assertEqual(len(native['anims'][0]['anim']['vec3'][0]['keys']), 2)

    def test_single_key_and_multiclip(self):
        f = self.custom('translation', 'LINEAR', [0.25], [(3, 4, 5)])
        for mode in ('STEP', 'LINEAR'):
            a = copy.deepcopy(f.model['animations'][0]); a['name'] = mode; a['samplers'][0]['interpolation'] = mode
            f.model['animations'].append(a)
        self.run_fixture(f)

    def test_nanosecond_intervals(self):
        self.run_fixture(self.custom('translation', 'LINEAR', [0, 2e-9, 5e-9], [(0, 0, 0), (1, 2, 3), (2, 4, 6)]))

    def test_invalid_animation_data(self):
        for case in ('bad_sampler', 'bad_node', 'duplicate_target', 'time_order', 'time_negative', 'time_nan', 'output_count', 'value_nan', 'zero_quaternion', 'mode'):
            f = self.custom('rotation', 'LINEAR', [0, 1], [(0, 0, 0, 1), (0, 0, 1, 0)])
            a = f.model['animations'][0]; sampler = a['samplers'][0]
            if case == 'bad_sampler': a['channels'][0]['sampler'] = 100
            elif case == 'bad_node': a['channels'][0]['target']['node'] = 100
            elif case == 'duplicate_target': a['channels'].append(copy.deepcopy(a['channels'][0]))
            elif case.startswith('time_'):
                vals = {'time_order': [1, 0], 'time_negative': [-1, 1], 'time_nan': [0, float('nan')]}[case]
                accessor_id = sampler['input']; view = f.model['bufferViews'][f.model['accessors'][accessor_id]['bufferView']]
                struct.pack_into('<2f', f.data, view['byteOffset'], *vals)
            elif case == 'output_count': f.model['accessors'][sampler['output']]['count'] = 1
            elif case in ('value_nan', 'zero_quaternion'):
                view = f.model['bufferViews'][f.model['accessors'][sampler['output']]['bufferView']]
                struct.pack_into('<4f', f.data, view['byteOffset'], *( [float('nan'), 0, 0, 1] if case == 'value_nan' else [0]*4))
            elif case == 'mode': sampler['interpolation'] = 'UNKNOWN'
            self.run_fixture(f, case, invalid=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--importer', type=Path, required=True)
    parser.add_argument('--sampler', type=Path, required=True)
    parser.add_argument('--exporter', type=Path, required=True, help='gltf_exporter_tests executable')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    AnimationTests.importer = args.importer.resolve()
    AnimationTests.sampler = args.sampler.resolve()
    AnimationTests.exporter = args.exporter.resolve()
    AnimationTests.output = Path(tempfile.mkdtemp(prefix='animation-', dir=args.out.resolve()))
    AnimationTests.metrics = {}
    print('Test artifacts:', AnimationTests.output, flush=True)
    result = unittest.main(argv=[__file__], verbosity=2, exit=False)
    (AnimationTests.output/'measurements.json').write_text(json.dumps(AnimationTests.metrics, indent=2)+'\n')
    raise SystemExit(not result.result.wasSuccessful())

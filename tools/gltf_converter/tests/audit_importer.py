#!/usr/bin/env python3
"""Generate diagnostic glTF fixtures and run the importer without changing it.

These are audit probes, not passing regression tests. Inspect measurements.json
and each output/log. Run with an empty --out directory; no assets are deleted.
Python standard library only. See AUDIT_GLTF_IMPORTER.md for interpretation.
"""
import argparse
import base64
import json
from pathlib import Path
import struct
import subprocess
import zlib


def geometry(path):
    data = path.read_bytes()
    assert struct.unpack_from('<I', data, 5)[0] == 2
    offset = 9
    result = {}
    for name, fmt in [('positions', '3f'), ('polygons', '2B'), ('bindings', 'I'),
                      ('normals', '3f'), ('colors', '4f'), ('tangents', '6f'),
                      *[(f'uv{i}', '2f') for i in range(8)],
                      ('skin', '4H4B'), ('bind_poses', '12f')]:
        count, = struct.unpack_from('<I', data, offset)
        offset += 4
        size = struct.calcsize('<' + fmt)
        result[name] = [list(struct.unpack_from('<' + fmt, data, offset + i * size)) for i in range(count)]
        offset += count * size
    assert offset == len(data)
    return result


class Fixture:
    def __init__(self):
        self.data = bytearray()
        self.model = {'asset': {'version': '2.0'}, 'scene': 0,
                      'scenes': [{'nodes': [0]}], 'nodes': [{'name': 'Mesh', 'mesh': 0}],
                      'accessors': [], 'bufferViews': [],
                      'materials': [{'name': 'Material'}]}
        p = self.add('VEC3', [(0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0)], bounds=True, target=34962)
        n = self.add('VEC3', [(0, 0, 1)] * 4, target=34962)
        i = self.add('SCALAR', [(0,), (1,), (2,), (2,), (1,), (3,)], 5123, target=34963)
        self.primitive = {'attributes': {'POSITION': p, 'NORMAL': n}, 'indices': i, 'material': 0, 'mode': 4}
        self.model['meshes'] = [{'name': 'Mesh', 'primitives': [self.primitive]}]

    def view(self, raw, target=None):
        self.data.extend(b'\0' * (-len(self.data) % 4))
        view = {'buffer': 0, 'byteOffset': len(self.data), 'byteLength': len(raw)}
        if target:
            view['target'] = target
        self.model['bufferViews'].append(view)
        self.data.extend(raw)
        return len(self.model['bufferViews']) - 1

    def add(self, kind, rows, component=5126, bounds=False, target=None, normalized=False):
        fmt = {5126: 'f', 5123: 'H', 5121: 'B', 5120: 'b'}[component]
        raw = b''.join(struct.pack('<' + fmt * len(row), *row) for row in rows)
        a = {'bufferView': self.view(raw, target), 'componentType': component, 'count': len(rows), 'type': kind}
        if normalized:
            a['normalized'] = True
        if bounds:
            a.update(min=list(map(min, zip(*rows))), max=list(map(max, zip(*rows))))
        self.model['accessors'].append(a)
        return len(self.model['accessors']) - 1

    def skin(self):
        a = self.primitive['attributes']
        a['JOINTS_0'] = self.add('VEC4', [(0, 1, 0, 0)] * 4, 5121, target=34962)
        a['WEIGHTS_0'] = self.add('VEC4', [(0.5, 0.5, 0, 0)] * 4, target=34962)
        identity = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
        ibm = self.add('MAT4', [identity, identity])
        self.model['skins'] = [{'joints': [1, 2], 'inverseBindMatrices': ibm}]
        self.model['nodes'][0]['skin'] = 0
        self.model['nodes'] += [{'name': 'Bone0', 'children': [2]}, {'name': 'Bone1'}]
        self.model['scenes'][0]['nodes'] += [1]

    def animation(self, interpolation='LINEAR'):
        times = self.add('SCALAR', [(0,), (1,)], bounds=True)
        values = [(0, 0, 2), (0, 0, 4)]
        if interpolation == 'CUBICSPLINE':
            values = [(0, 0, 0), values[0], (0, 0, 0), (0, 0, 0), values[1], (0, 0, 0)]
        output = self.add('VEC3', values)
        anim = {'name': interpolation, 'samplers': [{'input': times, 'output': output, 'interpolation': interpolation}],
                'channels': [{'sampler': 0, 'target': {'node': 0, 'path': 'translation'}}]}
        self.model['animations'] = [anim]
        return anim, times

    def texture(self):
        def chunk(tag, data):
            return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data))
        png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 2, 0, 0, 0))
        png += chunk(b'IDAT', zlib.compress(b'\0\xff\x80\x40')) + chunk(b'IEND', b'')
        self.model['images'] = [{'name': 'pixel', 'mimeType': 'image/png', 'bufferView': self.view(png)}]
        self.model['textures'] = [{'source': 0}]
        self.primitive['attributes']['TEXCOORD_0'] = self.add('VEC2', [(0, 0)] * 4, target=34962)

    def save(self, path):
        self.model['buffers'] = [{'byteLength': len(self.data), 'uri': 'data:application/octet-stream;base64,' + base64.b64encode(self.data).decode()}]
        path.write_text(json.dumps(self.model, indent=2) + '\n')


def fixtures():
    yield 'indexed', Fixture()
    f = Fixture(); del f.primitive['indices']
    f.model['accessors'][0]['count'] = f.model['accessors'][1]['count'] = 3
    yield 'nonindexed', f
    for mode in (5, 6):
        f = Fixture(); f.primitive['mode'] = mode
        f.primitive['indices'] = f.add('SCALAR', [(0,), (1,), (2,), (3,)], 5123, target=34963)
        yield 'strip' if mode == 5 else 'fan', f
    for interpolation in ('LINEAR', 'STEP', 'CUBICSPLINE'):
        f = Fixture(); f.animation(interpolation); yield interpolation.lower(), f
    f = Fixture(); a, times = f.animation()
    scale = f.add('VEC3', [(1, 1, 3), (2, 2, 5)])
    a['samplers'].append({'input': times, 'output': scale})
    a['channels'].insert(0, {'sampler': 1, 'target': {'node': 0, 'path': 'scale'}})
    yield 'reordered_channels', f
    f = Fixture(); a, times = f.animation()
    a['channels'].append({'sampler': 0, 'target': {'node': 0, 'path': 'scale'}})
    yield 'shared_sampler', f
    for mode in ('normal', 'no_ibm', 'instance', 'no_material', 'shear_ibm'):
        f = Fixture(); f.skin()
        if mode == 'no_ibm': del f.model['skins'][0]['inverseBindMatrices']
        if mode == 'instance':
            f.model['nodes'].append({'name': 'Instance', 'mesh': 0, 'skin': 0})
            f.model['scenes'][0]['nodes'].append(3)
        if mode == 'no_material': del f.primitive['material']
        if mode == 'shear_ibm':
            a = f.model['accessors'][f.model['skins'][0]['inverseBindMatrices']]
            offset = f.model['bufferViews'][a['bufferView']]['byteOffset']
            struct.pack_into('<f', f.data, offset + 4 * 4, 0.5)
        yield 'skin_' + mode, f
    for with_base in (True, False):
        f = Fixture(); a = f.model['accessors'][0]
        a['sparse'] = {'count': 1, 'indices': {'bufferView': f.view(b'\x01'), 'componentType': 5121},
                       'values': {'bufferView': f.view(struct.pack('<3f', 2, 0, 0))}}
        a['max'] = [2, 1 if with_base else 0, 0]
        if not with_base: del a['bufferView']
        yield 'sparse_base' if with_base else 'sparse_only', f
    f = Fixture(); f.primitive['attributes']['TANGENT'] = f.add('VEC4', [(0.6, 0, 0.8, 1)] * 4, target=34962)
    f.primitive['attributes']['NORMAL'] = f.add('VEC3', [(-0.8, 0, 0.6)] * 4, target=34962)
    yield 'tangents', f
    f = Fixture(); f.primitive['attributes']['COLOR_0'] = f.add('VEC4', [(1, 0, 0, 1)] * 4, target=34962)
    yield 'colors', f
    f = Fixture(); f.model.update(extensionsUsed=['KHR_mesh_quantization'], extensionsRequired=['KHR_mesh_quantization'])
    f.primitive['attributes']['NORMAL'] = f.add('VEC3', [(0, 0, 127)] * 4, 5120, normalized=True, target=34962)
    # Quantized VEC3 byte attributes require four-byte vertex alignment.
    a = f.model['accessors'][f.primitive['attributes']['NORMAL']]
    a['bufferView'] = f.view(bytes([0, 0, 127, 0]) * 4, 34962)
    f.model['bufferViews'][a['bufferView']]['byteStride'] = 4
    yield 'signed_normalized', f
    f = Fixture(); f.texture()
    f.model['materials'][0].update(occlusionTexture={'index': 0}, pbrMetallicRoughness={'roughnessFactor': 0.5, 'metallicFactor': 0.7})
    yield 'occlusion_only', f
    f = Fixture(); f.texture()
    f.model['materials'][0].update(alphaMode='MASK', alphaCutoff=0.2, pbrMetallicRoughness={'baseColorTexture': {'index': 0}, 'baseColorFactor': [0.2, 0.3, 0.4, 0.5]})
    yield 'mask_texture', f
    f = Fixture(); f.animation(); f.model['nodes'].append({'name': 'Other'})
    f.model['scenes'] = [{'name': 'A', 'nodes': [0]}, {'name': 'B', 'nodes': [1]}]
    yield 'multiple_scenes', f


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--importer', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    root = args.out.resolve()
    root.mkdir(parents=True, exist_ok=True)
    if any(root.iterdir()):
        parser.error('--out must be empty to avoid stale outputs and skip-policy effects')
    results = {}
    for name, fixture in fixtures():
        src = root / (name + '.gltf'); fixture.save(src)
        out = root / name; out.mkdir()
        command = [str(args.importer.resolve()), str(src), '-out', str(out), '-base-resource-path', str(out)]
        with (root / (name + '.log')).open('w') as log:
            try:
                run = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=30)
                code = run.returncode
            except subprocess.TimeoutExpired:
                code = 'timeout'
        results[name] = {'returncode': code, 'scenes': {}, 'geometry': {}, 'metadata': {}}
        for path in out.glob('*.scn'):
            doc = json.loads(path.read_text())
            results[name]['scenes'][path.name] = {key: doc.get(key, []) for key in ('nodes', 'objects', 'anims', 'scene_anims')}
        for path in out.glob('*.geo'):
            results[name]['geometry'][path.name] = geometry(path)
        for path in out.rglob('*.meta'):
            results[name]['metadata'][str(path.relative_to(out))] = json.loads(path.read_text())
        results[name]['files'] = sorted(str(p.relative_to(out)) for p in out.rglob('*') if p.is_file())
        print(name, code)
    (root / 'measurements.json').write_text(json.dumps(results, indent=2) + '\n')


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Compare expanded triangle attributes in two self-contained glTF/GLB exports."""
import argparse
import base64
import json
from pathlib import Path
import struct


def load(path):
    data = path.read_bytes()
    binary = None
    if data[:4] == b'glTF':
        magic, version, length = struct.unpack_from('<4sII', data)
        assert version == 2 and length == len(data)
        offset = 12
        while offset < len(data):
            size, kind = struct.unpack_from('<II', data, offset)
            chunk = data[offset + 8:offset + 8 + size]
            if kind == 0x4E4F534A:
                model = json.loads(chunk)
            elif kind == 0x004E4942:
                binary = chunk
            offset += 8 + size
    else:
        model = json.loads(data)
    buffers = []
    for buffer in model['buffers']:
        uri = buffer.get('uri')
        if uri is None:
            buffers.append(binary)
        elif uri.startswith('data:'):
            buffers.append(base64.b64decode(uri.split(',', 1)[1]))
        else:
            buffers.append((path.parent / uri).read_bytes())
    return model, buffers


def accessor(model, buffers, index):
    a = model['accessors'][index]
    assert 'sparse' not in a, 'Sparse accessors are outside this probe'
    v = model['bufferViews'][a['bufferView']]
    components = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[a['type']]
    fmt = '<' + {5121: 'B', 5123: 'H', 5125: 'I', 5126: 'f'}[a['componentType']] * components
    size = struct.calcsize(fmt)
    start = v.get('byteOffset', 0) + a.get('byteOffset', 0)
    return [struct.unpack_from(fmt, buffers[v['buffer']], start + i * v.get('byteStride', size))
            for i in range(a['count'])]


def compare(before_path, after_path):
    before, before_buffers = load(before_path)
    after, after_buffers = load(after_path)
    assert len(before['meshes']) == len(after['meshes']), 'Mesh count changed'
    for field in ('nodes', 'scenes', 'materials'):
        assert before.get(field) == after.get(field), f'{field} changed'
    old_positions, new_positions = set(), set()
    corners = 0
    for old_mesh, new_mesh in zip(before['meshes'], after['meshes']):
        assert old_mesh.get('name') == new_mesh.get('name')
        assert len(old_mesh['primitives']) == len(new_mesh['primitives'])
        for old, new in zip(old_mesh['primitives'], new_mesh['primitives']):
            assert old.get('mode', 4) == new.get('mode', 4) == 4
            assert old.get('material') == new.get('material')
            assert old['attributes'].keys() == new['attributes'].keys()
            oi = [x[0] for x in accessor(before, before_buffers, old['indices'])]
            ni = [x[0] for x in accessor(after, after_buffers, new['indices'])]
            assert len(oi) == len(ni) and len(oi) % 3 == 0
            corners += len(oi)
            old_positions.add(old['attributes']['POSITION'])
            new_positions.add(new['attributes']['POSITION'])
            for semantic in old['attributes']:
                ov = accessor(before, before_buffers, old['attributes'][semantic])
                nv = accessor(after, after_buffers, new['attributes'][semantic])
                assert all(0 <= i < len(ov) for i in oi)
                assert all(0 <= i < len(nv) for i in ni)
                assert [ov[i] for i in oi] == [nv[i] for i in ni], (old_mesh.get('name'), semantic)
    result = {
        'meshes': len(after['meshes']), 'triangle_corners_in_meshes': corners,
        'vertices_before': sum(before['accessors'][i]['count'] for i in old_positions),
        'vertices_after': sum(after['accessors'][i]['count'] for i in new_positions),
        'bytes_before': before_path.stat().st_size, 'bytes_after': after_path.stat().st_size,
        'expanded_attributes_equal': True,
    }
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=Path)
    parser.add_argument('after', type=Path)
    args = parser.parse_args()
    compare(args.before, args.after)

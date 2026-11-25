#!/usr/bin/env python3
"""Compare Colossus glTF skinning with the C++ native evaluator reference.

Requires NumPy. Reference samples use normalized native weights, matching the
exporter's explicit normalization policy. Full playback error also includes the
remaining native-Hermite/glTF-linear interpolation difference.
"""
import argparse
import json
import struct
from pathlib import Path
import numpy as np
from compare_exports import load, accessor


def native_geometry(path):
    data = path.read_bytes()
    offset = 9
    assert struct.unpack_from('<I', data, 5)[0] == 2

    def read(fmt):
        nonlocal offset
        count = struct.unpack_from('<I', data, offset)[0]
        offset += 4
        size = struct.calcsize('<' + fmt)
        values = [struct.unpack_from('<' + fmt, data, offset + i * size) for i in range(count)]
        offset += count * size
        return values

    positions, polygons, bindings = read('3f'), read('2B'), read('I')
    read('3f')  # normals
    read('4f')  # colors
    read('6f')  # tangent frames
    for _ in range(8):
        read('2f')
    weights, bind_poses = read('4H4B'), read('12f')
    assert offset == len(data)
    return positions, polygons, bindings, weights, bind_poses


def affine(values):
    matrix = np.eye(4)
    matrix[:3] = np.asarray(values).reshape(3, 4)
    return matrix


def trs(position, rotation, scale):
    x, y, z, w = np.asarray(rotation) / np.linalg.norm(rotation)
    matrix = np.eye(4)
    matrix[:3, :3] = np.array([
        [1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
        [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
        [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)],
    ]) @ np.diag(scale)
    matrix[:3, 3] = position
    return matrix


def sample(times, values, time, rotation):
    upper = np.searchsorted(times, time, side='right')
    if upper == 0:
        return values[0]
    if upper == len(times):
        return values[-1]
    alpha = (time-times[upper-1]) / (times[upper]-times[upper-1])
    a, b = values[upper-1], values[upper]
    if not rotation:
        return a*(1-alpha) + b*alpha
    a, b = a/np.linalg.norm(a), b/np.linalg.norm(b)
    dot = np.dot(a, b)
    if dot < 0:
        b, dot = -b, -dot
    if dot > 0.9995:
        q = a*(1-alpha) + b*alpha
        return q/np.linalg.norm(q)
    angle = np.arccos(np.clip(dot, -1, 1))
    return (a*np.sin((1-alpha)*angle) + b*np.sin(alpha*angle)) / np.sin(angle)


def check(source_scene, geometry, reference_path, exported):
    source = json.loads(source_scene.read_text())
    reference = json.loads(reference_path.read_text())
    model, buffers = load(exported)
    positions, polygons, bindings, native_weights, bind_poses = native_geometry(geometry)
    mesh_node = next(n for n in model['nodes'] if 'mesh' in n)
    skin = model['skins'][mesh_node['skin']]
    primitive = model['meshes'][mesh_node['mesh']]['primitives'][0]
    get = lambda index: np.asarray(accessor(model, buffers, index))
    attributes = primitive['attributes']
    points = get(attributes['POSITION'])
    joints = get(attributes['JOINTS_0']).astype(int)
    weights = get(attributes['WEIGHTS_0'])
    ibm = get(skin['inverseBindMatrices']).reshape(-1, 4, 4).transpose(0, 2, 1)
    assert len(set(skin['joints'])) == len(skin['joints'])
    assert joints.min() >= 0 and joints.max() < len(skin['joints'])
    assert np.allclose(weights.sum(axis=1), 1, atol=1e-7, rtol=0)
    # Recover exact native identity from polygon bindings and output triangle indices.
    corners, base = [], 0
    for count, material in polygons:
        assert material == 0
        for k in range(1, count-1):
            corners.extend([base, base+k+1, base+k])
        base += count
    assert len(weights) == len(joints) == len(points)
    source_ids = np.full(len(points), -1, dtype=int)
    indices = get(primitive['indices']).reshape(-1).astype(int)
    assert len(indices) == len(corners)
    for out, corner in zip(indices, corners):
        source_id = bindings[corner][0]
        assert source_ids[out] in (-1, source_id)
        source_ids[out] = source_id
    assert (source_ids >= 0).all()
    reflection = np.diag([1, 1, -1, 1])
    assert np.array_equal(points, np.asarray(positions)[source_ids] * [1, 1, -1])
    nodes = {n['idx']: n for n in source['nodes']}
    bone_ids = source['objects'][0]['bones']
    joint_names = [model['nodes'][i]['name'] for i in skin['joints']]
    for slot, pose in enumerate(bind_poses):
        joint = joint_names.index(nodes[bone_ids[slot]]['name'])
        assert np.array_equal(ibm[joint], reflection @ affine(pose) @ reflection)
    for vertex, source_id in enumerate(source_ids):
        row = native_weights[source_id]
        expected = np.zeros(len(joint_names))
        for slot, weight in zip(row[:4], row[4:]):
            expected[joint_names.index(nodes[bone_ids[slot]]['name'])] += weight
        expected /= expected.sum()
        actual = np.zeros(len(joint_names))
        np.add.at(actual, joints[vertex], weights[vertex])
        assert np.allclose(actual, expected, atol=3e-8, rtol=0)

    homogeneous = np.column_stack([points, np.ones(len(points))])

    def deform(worlds):
        matrices = np.asarray(worlds) @ ibm
        return np.einsum('vijk,vk,vi->vj', matrices[joints], homogeneous, weights)[:, :3]

    parents = {child: i for i, node in enumerate(model['nodes']) for child in node.get('children', [])}
    assert len(model['animations']) == len(reference) == len(source['scene_anims'])
    result = {'vertices': len(points), 'joints': len(joint_names), 'clips': []}
    for clip, native in zip(model['animations'], reference):
        assert clip['name'] == native['clip']
        streams = []
        for channel in clip['channels']:
            sampler = clip['samplers'][channel['sampler']]
            assert sampler.get('interpolation', 'LINEAR') == 'LINEAR'
            streams.append((channel['target']['node'], channel['target']['path'],
                            get(sampler['input']).reshape(-1), get(sampler['output'])))
        exact_errors, playback_errors, poses = [], [], []
        for ref in native['samples']:
            expected = np.asarray(ref['positions'])[source_ids]
            native_worlds = [reflection @ affine(ref['joint_worlds'][name]) @ reflection for name in joint_names]
            exact_errors.extend(np.linalg.norm(deform(native_worlds)-expected, axis=1))
            state = [{k: np.asarray(node.get(k, default)) for k, default in
                      [('translation', [0, 0, 0]), ('rotation', [0, 0, 0, 1]), ('scale', [1, 1, 1])]}
                     for node in model['nodes']]
            for node, path, times, values in streams:
                state[node][path] = sample(times, values, ref['time'], path == 'rotation')
            worlds = {}

            def world(node):
                if node not in worlds:
                    s = state[node]
                    local = trs(s['translation'], s['rotation'], s['scale'])
                    worlds[node] = world(parents[node]) @ local if node in parents else local
                return worlds[node]

            animated = deform([world(i) for i in skin['joints']])
            playback_errors.extend(np.linalg.norm(animated-expected, axis=1))
            poses.append(animated)
        exact_max = float(max(exact_errors))
        assert exact_max < 2e-6, (clip['name'], 'skin deformation mismatch', exact_max)
        result['clips'].append({
            'name': clip['name'], 'samples': len(poses), 'native_matrix_skin_max_error': exact_max,
            'full_playback_max_error': float(max(playback_errors)),
            'full_playback_rms_error': float(np.sqrt(np.mean(np.square(playback_errors)))),
            'max_motion_from_first_pose': float(max(np.linalg.norm(p-poses[0], axis=1).max() for p in poses)),
        })
    print(json.dumps(result, indent=2))
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ('source_scene', 'geometry', 'reference', 'exported'):
        parser.add_argument(arg, type=Path)
    args = parser.parse_args()
    check(args.source_scene, args.geometry, args.reference, args.exported)

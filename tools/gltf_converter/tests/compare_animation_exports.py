#!/usr/bin/env python3
"""Verify complete TRS fixtures against a native scene and an earlier export.

Each native node binding is expected to contain position, scale and rotation.
"""
import argparse
import collections
import json
from pathlib import Path

from compare_exports import load, accessor, compare


def check(source_path, before_path, after_path):
    source = json.loads(source_path.read_text())
    model, buffers = load(after_path)
    old_model, old_buffers = load(before_path)
    nodes = {node['idx']: node for node in source['nodes']}
    tracks_by_id = {entry['idx']: entry['anim'] for entry in source['anims']}
    assert len(source['scene_anims']) == len(model['animations']) == len(old_model['animations'])
    report = []
    for source_clip, clip, old_clip in zip(source['scene_anims'], model['animations'], old_model['animations']):
        assert source_clip['name'] == clip['name'] == old_clip['name']
        channels = {
            (model['nodes'][c['target']['node']]['name'], c['target']['path']): clip['samplers'][c['sampler']]
            for c in clip['channels']
        }
        assert len(channels) == len(clip['channels']) == 3 * len(source_clip['node_anims'])
        for binding in source_clip['node_anims']:
            tracks = tracks_by_id[binding['anim']]
            name = nodes[binding['node']]['name']
            assert (name, 'rotation') in channels
            rotation = model['accessors'][channels[name, 'rotation']['output']]
            assert rotation['type'] == 'VEC4'
            for target, path in [('Position', 'translation'), ('Scale', 'scale')]:
                track = next(t for t in reversed(tracks['vec3']) if t['target'] == target)
                sampler = channels[name, path]
                expected = [
                    tuple(key['v'][:2] + [-key['v'][2]]) if target == 'Position' else tuple(key['v'])
                    for key in track['keys']
                ]
                assert accessor(model, buffers, sampler['output']) == expected, (name, path)
                times = accessor(model, buffers, sampler['input'])
                assert len(times) == len(track['keys'])
                assert all(abs(t[0] - key['t'] / 1e9) < 1e-5 for t, key in zip(times, track['keys']))

        # Existing selected quaternion samples and times must remain unchanged.
        for channel in old_clip['channels']:
            if channel['target']['path'] != 'rotation':
                continue
            name = old_model['nodes'][channel['target']['node']]['name']
            old_sampler = old_clip['samplers'][channel['sampler']]
            new_sampler = channels[name, 'rotation']
            for stream in ('input', 'output'):
                assert accessor(old_model, old_buffers, old_sampler[stream]) == accessor(model, buffers, new_sampler[stream])
        report.append({
            'clip': clip['name'],
            'channels': dict(collections.Counter(c['target']['path'] for c in clip['channels'])),
        })

    compare(before_path, after_path)  # Geometry, nodes, scene and materials remain unchanged.
    print(json.dumps(report, indent=2))
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('before', type=Path)
    parser.add_argument('after', type=Path)
    args = parser.parse_args()
    check(args.source, args.before, args.after)

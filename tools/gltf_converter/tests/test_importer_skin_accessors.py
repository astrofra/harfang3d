#!/usr/bin/env python3
"""CLI regressions for sparse accessors and skin import (no renderer required)."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

from audit_importer import Fixture, fixtures, geometry


class ImporterTests(unittest.TestCase):
    def run_fixture(self, fixture, suffix='', error=None):
        folder = self.output / (self._testMethodName + suffix)
        folder.mkdir()
        source = folder / 'input.gltf'
        fixture.save(source)
        out = folder / 'out'
        run = subprocess.run([str(self.importer), str(source), '-out', str(out),
                              '-base-resource-path', str(out)], capture_output=True, text=True, timeout=30)
        (folder / 'import.log').write_text(run.stdout + run.stderr)
        if error:
            self.assertEqual(run.returncode, 1, run.stdout + run.stderr)
            self.assertIn(error, run.stdout)
            self.assertFalse(list(out.glob('*.scn')))
            return
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        scenes = {p.stem: json.loads(p.read_text()) for p in out.glob('*.scn')}
        geos = {p.name: geometry(p) for p in out.glob('*.geo')}
        return scenes, geos

    def audit_case(self, name):
        return next(f for n, f in fixtures() if n == name)

    def sparse(self, fixture, accessor, indices, values, component=5121, zero_base=False):
        a = fixture.model['accessors'][accessor]
        fmt = {5121: 'B', 5123: 'H', 5125: 'I'}[component]
        # Deliberate offsets exercise both sparse offset fields.
        offset = struct.calcsize(fmt)
        a['sparse'] = {'count': len(indices),
                       'indices': {'bufferView': fixture.view(b'\0' * offset + struct.pack('<' + fmt * len(indices), *indices)),
                                   'byteOffset': offset, 'componentType': component},
                       'values': {'bufferView': fixture.view(b'\0' * 4 + values), 'byteOffset': 4}}
        if zero_base:
            a.pop('bufferView', None)

    def test_indexed_geometry_unchanged(self):
        _, geos = self.run_fixture(Fixture())
        g = geos['Mesh.geo']
        self.assertEqual(g['positions'], [[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0]])
        self.assertEqual(g['bindings'], [[2], [1], [0], [3], [1], [2]])

    def test_identity_bind_default(self):
        scenes, geos = self.run_fixture(self.audit_case('skin_no_ibm'))
        self.assertEqual(geos['Mesh.geo']['bind_poses'], [[1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0]] * 2)
        self.assertIn('EnableSkinning', scenes['input']['objects'][0]['materials'][0]['flags'])

    def test_instances_and_fallback_materials(self):
        for name in ('skin_instance', 'skin_no_material'):
            with self.subTest(name=name):
                scenes, geos = self.run_fixture(self.audit_case(name), name)
                objects = scenes['input']['objects']
                self.assertEqual(len(geos), 1)
                for obj in objects:
                    self.assertEqual(obj['bones'], [1, 2])
                    self.assertEqual(obj['name'], 'Mesh.geo')
                    self.assertTrue(all('EnableSkinning' in mat['flags'] for mat in obj['materials']))

    def test_affine_bind_basis_conversion(self):
        f = Fixture(); f.skin()
        # Rotation/shear, reflection, nonuniform scale and translation together.
        matrix = [0, 2, 0, 0, -3, 0.5, 0.25, 0, 0.75, 0, -4, 0, 5, 6, 7, 1]
        f.model['skins'][0]['inverseBindMatrices'] = f.add('MAT4', [matrix] * 2)
        _, geos = self.run_fixture(f)
        expected = [matrix[c * 4 + r] * (-1 if r == 2 else 1) * (-1 if c == 2 else 1)
                    for r in range(3) for c in range(4)]
        self.assertEqual(geos['Mesh.geo']['bind_poses'], [expected] * 2)

    def test_sparse_positions_with_and_without_base(self):
        for name in ('sparse_base', 'sparse_only'):
            with self.subTest(name=name):
                _, geos = self.run_fixture(self.audit_case(name), name)
                expected = [[0, 0, 0], [2, 0, 0], [0, 1, 0], [1, 1, 0]] if name == 'sparse_base' else [[0, 0, 0], [2, 0, 0], [0, 0, 0], [0, 0, 0]]
                self.assertEqual(geos['Mesh.geo']['positions'], expected)

    def test_interleaved_base_and_sparse_index_types(self):
        for component in (5121, 5123, 5125):
            with self.subTest(component=component):
                f = Fixture()
                raw = b'\0' * 4 + b''.join(struct.pack('<4f', x, y, 0, 99) for x, y in [(0, 0), (1, 0), (0, 1), (1, 1)])
                a = f.model['accessors'][0]
                a['bufferView'] = f.view(raw[:-4], 34962)
                a['byteOffset'] = 4
                f.model['bufferViews'][a['bufferView']]['byteStride'] = 16
                self.sparse(f, 0, [1, 3], struct.pack('<6f', 2, 0, 3, 4, 5, 6), component)
                a.update(min=[0, 0, 0], max=[4, 5, 6])
                _, geos = self.run_fixture(f, str(component))
                self.assertEqual(geos['Mesh.geo']['positions'], [[0, 0, 0], [2, 0, -3], [0, 1, 0], [4, 5, -6]])

    def test_sparse_triangle_indices(self):
        f = Fixture()
        self.sparse(f, f.primitive['indices'], [1], struct.pack('<H', 3))
        _, geos = self.run_fixture(f)
        self.assertEqual(geos['Mesh.geo']['bindings'][:3], [[2], [3], [0]])

    def test_sparse_animation(self):
        f = Fixture(); animation, times = f.animation()
        self.sparse(f, times, [1], struct.pack('<f', 2))
        f.model['accessors'][times]['max'] = [2]
        self.sparse(f, animation['samplers'][0]['output'], [1], struct.pack('<3f', 3, 4, 8))
        scenes, _ = self.run_fixture(f)
        keys = scenes['input']['anims'][0]['anim']['vec3'][0]['keys']
        self.assertEqual(keys[1]['t'], 2000000000)
        self.assertEqual(keys[1]['v'], [3, 4, -8])

    def test_sparse_skin_attributes_and_bind_matrices(self):
        f = Fixture(); f.skin()
        attrs = f.primitive['attributes']
        attrs['WEIGHTS_0'] = f.add('VEC4', [(1, 0, 0, 0)] * 4, target=34962)
        self.sparse(f, attrs['JOINTS_0'], [1], bytes([1, 0, 0, 0]), zero_base=True)
        self.sparse(f, attrs['WEIGHTS_0'], [1], struct.pack('<4f', 0.75, 0.25, 0, 0))
        matrix = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 2, 3, 1]
        self.sparse(f, f.model['skins'][0]['inverseBindMatrices'], [1], struct.pack('<16f', *matrix))
        _, geos = self.run_fixture(f)
        self.assertEqual(geos['Mesh.geo']['skin'][1], [1, 0, 0, 0, 191, 64, 0, 0])
        self.assertEqual(geos['Mesh.geo']['bind_poses'][1], [1, 0, 0, 1, 0, 1, 0, 2, 0, 0, 1, -3])

    def test_sparse_only_bind_matrices(self):
        f = Fixture(); f.skin()
        matrix = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 2, 3, 1]
        self.sparse(f, f.model['skins'][0]['inverseBindMatrices'], [0, 1], struct.pack('<32f', *(matrix * 2)), zero_base=True)
        _, geos = self.run_fixture(f)
        self.assertEqual(geos['Mesh.geo']['bind_poses'], [[1, 0, 0, 1, 0, 1, 0, 2, 0, 0, 1, -3]] * 2)

    def test_zero_initialized_dense_base(self):
        f = Fixture()
        del f.model['accessors'][0]['bufferView']
        f.model['accessors'][0].update(min=[0, 0, 0], max=[0, 0, 0])
        _, geos = self.run_fixture(f)
        self.assertEqual(geos['Mesh.geo']['positions'], [[0, 0, 0]] * 4)

    def test_normalized_integer_weights(self):
        for component, value in ((5121, 255), (5123, 65535)):
            f = Fixture(); f.skin()
            f.primitive['attributes']['JOINTS_0'] = f.add('VEC4', [(0, 0, 0, 0)] * 4, 5121, target=34962)
            f.primitive['attributes']['WEIGHTS_0'] = f.add('VEC4', [(value, 0, 0, 0)] * 4, component, normalized=True, target=34962)
            _, geos = self.run_fixture(f, str(component))
            self.assertTrue(all(row[4:] == [255, 0, 0, 0] for row in geos['Mesh.geo']['skin']))

    def test_scene_node_maps_are_isolated(self):
        scenes, _ = self.run_fixture(self.audit_case('multiple_scenes'))
        self.assertEqual(scenes['input_A']['scene_anims'][0]['node_anims'], [{'anim': 0, 'node': 0}])
        self.assertFalse(scenes['input_B']['scene_anims'][0]['node_anims'])

    def test_skin_in_other_scene_is_not_bound(self):
        f = Fixture(); f.skin()
        f.model['nodes'].append({'name': 'Other'})
        f.model['scenes'][0]['name'] = 'A'
        f.model['scenes'].append({'name': 'B', 'nodes': [3]})
        scenes, _ = self.run_fixture(f)
        self.assertFalse(scenes['input_B'].get('objects'))
        self.assertEqual(scenes['input_A']['objects'][0]['bones'], [1, 2])

    def test_malformed_sparse_ranges_and_indices(self):
        for name, indices, values in [('out_of_range', [4], [0, 0, 0]), ('duplicate', [1, 1], [0] * 6),
                                      ('descending', [2, 1], [0] * 6), ('short_values', [1], [0])]:
            with self.subTest(name=name):
                f = Fixture()
                self.sparse(f, 0, indices, struct.pack('<' + 'f' * len(values), *values))
                self.run_fixture(f, name, error='Accessor 0:')

    def test_malformed_dense_range(self):
        f = Fixture()
        f.model['accessors'][0]['count'] = 1000
        self.run_fixture(f, error='Accessor 0:')

    def test_invalid_skin_references_and_data(self):
        for name in ('accessor', 'count', 'joint_node', 'joint_missing', 'influence', 'missing_weights', 'weight_count', 'zero_weights', 'nan_matrix'):
            with self.subTest(name=name):
                f = Fixture(); f.skin()
                skin = f.model['skins'][0]
                attrs = f.primitive['attributes']
                if name == 'accessor': skin['inverseBindMatrices'] = 10000
                elif name == 'count': f.model['accessors'][skin['inverseBindMatrices']]['count'] = 1
                elif name == 'joint_node': skin['joints'][1] = 10000
                elif name == 'joint_missing': del f.model['nodes'][1]['children']
                elif name == 'influence': attrs['JOINTS_0'] = f.add('VEC4', [(2, 0, 0, 0)] * 4, 5121)
                elif name == 'missing_weights': del attrs['WEIGHTS_0']
                elif name == 'weight_count': f.model['accessors'][attrs['WEIGHTS_0']]['count'] = 3
                elif name == 'zero_weights': attrs['WEIGHTS_0'] = f.add('VEC4', [(0, 0, 0, 0)] * 4)
                elif name == 'nan_matrix':
                    view = f.model['bufferViews'][f.model['accessors'][skin['inverseBindMatrices']]['bufferView']]
                    struct.pack_into('<f', f.data, view['byteOffset'], float('nan'))
                self.run_fixture(f, name, error='Import failed:')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--importer', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    ImporterTests.importer = args.importer.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    ImporterTests.output = Path(tempfile.mkdtemp(prefix='skin-accessors-', dir=args.out.resolve()))
    print('Test artifacts:', ImporterTests.output, flush=True)
    unittest.main(argv=[__file__], verbosity=2)

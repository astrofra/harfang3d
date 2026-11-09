"""Exercise real assetc -> toolchain/gsplatc conversion and incremental builds."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import unittest


FIXTURES = Path(__file__).resolve().parents[3] / 'harfang/tests/test_gaussian_splat_native.py'
spec = importlib.util.spec_from_file_location('native_fixtures', FIXTURES)
native = importlib.util.module_from_spec(spec)
spec.loader.exec_module(native)


class GaussianAssetTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp(prefix=self._testMethodName + ' ', dir=OUTPUT))
        self.source, self.compiled = self.root / 'source assets', self.root / 'compiled assets'
        self.source.mkdir()
        self.ply = self.source / 'island sample.ply'
        self.meta = self.ply.with_suffix('.ply.meta')
        self.hgs = self.compiled / 'island sample.hgs'
        native.write_ply(self.ply, [native.make_record(i) for i in range(7)])
        self.counter = 0

    def metadata(self, **overrides):
        settings = {'type': 'gaussian-splat', 'max-count': 3, 'sh-degree': 1, 'normalize': False}
        settings.update(overrides)
        self.meta.write_text(json.dumps({'profiles': {'default': settings}}), encoding='utf-8')

    def invoke(self, *extra, success=True, toolchain=None):
        self.counter += 1
        command = [str(ASSETC), '-toolchain', str(toolchain or TOOLCHAIN), *extra,
                   str(self.source), str(self.compiled)]
        result = subprocess.run(command, capture_output=True, text=True, errors='replace', timeout=60)
        log = result.stdout + result.stderr
        (self.root / f'run-{self.counter}.log').write_text(log, encoding='utf-8')
        self.assertEqual(result.returncode == 0, success, log)
        return log

    def test_two_gaussian_source_formats_cannot_share_output(self):
        self.metadata()
        sibling = self.ply.with_suffix('.splat')
        sibling.write_bytes(native.splat_bytes())
        sibling.with_suffix('.splat.meta').write_text(self.meta.read_text())
        log = self.invoke(success=False)
        self.assertIn('GaussianSplatOutputCollision', log)
        self.assertFalse(self.hgs.exists())

    def test_spz_metadata_profiles_and_passthrough(self):
        self.ply.unlink()
        self.ply = self.source / 'island.spz'
        self.ply.write_bytes(native.spz_bytes())
        self.meta = self.ply.with_suffix('.spz.meta')
        self.hgs = self.compiled / 'island.hgs'
        self.invoke()
        self.assertEqual((self.compiled / self.ply.name).read_bytes(), self.ply.read_bytes())
        self.metadata()
        self.invoke()
        data = native.read_hgs(self.hgs)
        self.assertEqual((data['splat_count'], data['source_count'], data['sh_degree'], data['flags']), (3, 7, 1, 5))
        self.assertFalse((self.compiled / self.ply.name).exists())
        direct = self.root / 'direct.hgs'
        subprocess.run([str(GSPLATC), str(self.ply), str(direct), '--max-count', '3', '--sh-degree', '1'],
                       check=True, capture_output=True, timeout=60)
        self.assertEqual(self.hgs.read_bytes(), direct.read_bytes())
        stamp = self.hgs.stat().st_mtime_ns
        self.invoke()
        self.assertEqual(stamp, self.hgs.stat().st_mtime_ns)
        meta = json.loads(self.meta.read_text())
        meta['profiles']['preview'] = {'max-count': 2, 'sh-degree': 0}
        self.meta.write_text(json.dumps(meta))
        self.invoke('-profile', 'preview')
        data = native.read_hgs(self.hgs)
        self.assertEqual((data['splat_count'], data['sh_degree']), (2, 0))
        # SPZ / PLY collision must also be detected (different compressed dialect).
        sibling = self.ply.with_suffix('.ply')
        native.write_ply(sibling, [native.make_record(i) for i in range(7)])
        sibling.with_suffix('.ply.meta').write_text(self.meta.read_text())
        self.assertIn('GaussianSplatOutputCollision', self.invoke(success=False))
        sibling.with_suffix('.ply.meta').unlink()
        sibling.unlink()
        self.meta.unlink()
        self.invoke()
        self.assertFalse(self.hgs.exists())
        self.assertEqual((self.compiled / self.ply.name).read_bytes(), self.ply.read_bytes())

    def test_binary_splat_optin_and_passthrough(self):
        self.ply.unlink()
        self.ply = self.source / 'plant.splat'
        self.ply.write_bytes(native.splat_bytes())
        self.meta = self.ply.with_suffix('.splat.meta')
        self.hgs = self.compiled / 'plant.hgs'
        self.invoke()
        self.assertEqual((self.compiled / 'plant.splat').read_bytes(), self.ply.read_bytes())
        self.metadata()
        self.invoke()
        data = native.read_hgs(self.hgs)
        self.assertEqual((data['splat_count'], data['source_count'], data['sh_degree']), (3, 7, 0))
        self.assertFalse((self.compiled / 'plant.splat').exists())
        direct = self.root / 'direct.hgs'
        subprocess.run([str(GSPLATC), str(self.ply), str(direct), '--max-count', '3'],
                       check=True, capture_output=True, timeout=60)
        self.assertEqual(self.hgs.read_bytes(), direct.read_bytes())
        self.meta.unlink()
        self.invoke()
        self.assertFalse(self.hgs.exists())
        self.assertTrue((self.compiled / 'plant.splat').exists())

    def test_conversion_matches_native_tool_and_incremental_rebuild(self):
        nested = self.source / 'nested folder'
        nested.mkdir()
        self.ply = self.ply.rename(nested / self.ply.name)
        self.meta = self.ply.with_suffix('.ply.meta')
        self.hgs = self.compiled / 'nested folder' / self.hgs.name
        self.metadata()
        self.invoke()
        data = native.read_hgs(self.hgs)
        self.assertEqual((data['splat_count'], data['source_count'], data['sh_degree']), (3, 7, 1))
        self.assertFalse(self.hgs.with_suffix('.ply').exists())
        direct = self.root / 'direct.hgs'
        subprocess.run([str(GSPLATC), str(self.ply), str(direct), '--max-count', '3', '--sh-degree', '1'],
                       check=True, capture_output=True, timeout=60)
        self.assertEqual(self.hgs.read_bytes(), direct.read_bytes())
        stamp = self.hgs.stat().st_mtime_ns
        log = self.invoke()
        self.assertEqual(self.hgs.stat().st_mtime_ns, stamp, 'Unchanged Gaussian asset was rebuilt')
        self.assertNotIn('Spawning compile process', log)
        self.metadata(**{'max-count': 5, 'sh-degree': 0, 'normalize': True})
        self.invoke()
        data = native.read_hgs(self.hgs)
        self.assertEqual((data['splat_count'], data['sh_degree'], data['flags'] & 2), (5, 0, 2))
        records = [native.make_record(i) for i in range(9)]
        native.write_ply(self.ply, records)
        self.invoke()
        self.assertEqual(native.read_hgs(self.hgs)['source_count'], 9)

    def test_profile_overrides_inherit_individual_defaults(self):
        self.metadata()
        meta = json.loads(self.meta.read_text())
        meta['profiles']['preview'] = {'max-count': 2, 'normalize': True}
        self.meta.write_text(json.dumps(meta))
        self.invoke('-profile', 'preview')
        data = native.read_hgs(self.hgs)
        self.assertEqual((data['splat_count'], data['sh_degree'], data['flags'] & 2), (2, 1, 2))
        self.invoke('-profile', 'default')
        data = native.read_hgs(self.hgs)
        self.assertEqual((data['splat_count'], data['sh_degree'], data['flags'] & 2), (3, 1, 0))

    def test_builtin_defaults_and_non_gaussian_passthrough(self):
        self.invoke()
        self.assertEqual((self.compiled / self.ply.name).read_bytes(), self.ply.read_bytes())
        self.assertFalse(self.hgs.exists())
        self.meta.write_text(json.dumps({'profiles': {'default': {'type': 'gaussian-splat'}}}))
        self.invoke()
        data = native.read_hgs(self.hgs)
        self.assertEqual((data['splat_count'], data['sh_degree'], data['flags'] & 2), (7, 3, 0))
        self.assertFalse((self.compiled / self.ply.name).exists(), 'Stale pass-through PLY remained')
        self.meta.unlink()
        self.invoke()
        self.assertFalse(self.hgs.exists(), 'Stale Gaussian output remained after metadata removal')
        self.assertEqual((self.compiled / self.ply.name).read_bytes(), self.ply.read_bytes())
        self.meta.write_text(json.dumps({'profiles': {'default': {'type': 'mesh', 'other': 123}}}))
        self.invoke()
        self.assertFalse(self.hgs.exists())

    def test_invalid_metadata_fails_and_recovers(self):
        invalid = ['{', 'null', '[]', '{"profiles": []}', '{"profiles":{"default":[]}}',
                   '{"profiles":{"default":{"type":"gaussian-splat","max-count":1e999}}}']
        for field, value in [('max-count', 0), ('max-count', 1048577), ('max-count', True),
                             ('max-count', 2.5), ('max-count', 18446744073709551616),
                             ('sh-degree', -1), ('sh-degree', 4), ('sh-degree', '3'),
                             ('normalize', 1), ('type', 42), ('max_count', 3)]:
            self.metadata(**{field: value})
            invalid.append(self.meta.read_text())
        for contents in invalid:
            with self.subTest(contents=contents):
                self.meta.write_text(contents)
                log = self.invoke(success=False)
                self.assertNotIn('Spawning compile process', log)
                self.assertIn('InvalidGaussianSplatMetadata', log)
        self.metadata()
        meta = json.loads(self.meta.read_text())
        meta['profiles']['preview'] = {'max-count': '2'}
        self.meta.write_text(json.dumps(meta))
        self.invoke('-profile', 'preview', success=False)
        self.metadata()
        self.invoke()
        self.assertEqual(native.read_hgs(self.hgs)['splat_count'], 3)

    def test_missing_tool_only_fails_opted_in_assets(self):
        missing = self.root / 'empty toolchain'
        missing.mkdir()
        self.invoke(toolchain=missing)
        self.metadata()
        self.invoke(toolchain=missing, success=False)
        self.invoke()
        self.assertTrue(self.hgs.is_file())

    def test_failed_conversion_is_retried_and_removed_source_cleans_output(self):
        self.metadata()
        self.invoke()
        self.ply.write_bytes(b'not a PLY')
        self.invoke(success=False)
        self.assertFalse(self.hgs.exists(), 'Failed conversion left a stale runtime asset')
        self.invoke(success=False)  # Same invalid source must not become an incremental success.
        native.write_ply(self.ply, [native.make_record(i) for i in range(7)])
        self.invoke()
        self.assertEqual(native.read_hgs(self.hgs)['splat_count'], 3)
        self.ply.unlink()
        self.invoke()
        self.assertFalse(self.hgs.exists())

    def test_output_name_collision_fails(self):
        self.metadata()
        (self.source / self.hgs.name).write_bytes(b'independent native source asset')
        self.invoke(success=False)

    def test_daemon_reacts_to_metadata_edit_and_removal(self):
        self.metadata()
        command = [str(ASSETC), '-daemon', '-toolchain', str(TOOLCHAIN), str(self.source), str(self.compiled)]
        with (self.root / 'daemon.log').open('w', encoding='utf-8') as log:
            process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
            try:
                def wait_for(predicate):
                    deadline = time.monotonic() + 20
                    while time.monotonic() < deadline:
                        self.assertIsNone(process.poll(), 'assetc daemon exited; see daemon.log')
                        try:
                            if predicate():
                                return
                        except (OSError, native.TestFailure):
                            pass  # Output may be in the process of being written.
                        time.sleep(0.1)
                    self.fail('assetc daemon did not react within 20 seconds; see daemon.log')
                wait_for(lambda: self.hgs.is_file() and native.read_hgs(self.hgs)['splat_count'] == 3)
                # Watch registration follows the initial compilation; let it become active.
                time.sleep(1.2)
                self.metadata(**{'max-count': 2})
                wait_for(lambda: self.hgs.is_file() and native.read_hgs(self.hgs)['splat_count'] == 2)
                self.meta.unlink()
                wait_for(lambda: not self.hgs.exists() and (self.compiled / self.ply.name).is_file())
            finally:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assetc', required=True, type=Path)
    parser.add_argument('--gsplatc', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    ASSETC, GSPLATC, OUTPUT = args.assetc.resolve(), args.gsplatc.resolve(), args.output.resolve()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    TOOLCHAIN = OUTPUT / 'toolchain with spaces'
    TOOLCHAIN.mkdir(exist_ok=True)
    shutil.copy2(GSPLATC, TOOLCHAIN / ('gsplatc.exe' if os.name == 'nt' else 'gsplatc'))
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(GaussianAssetTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    (OUTPUT / 'summary.json').write_text(json.dumps({'passed': result.wasSuccessful(),
        'tests': result.testsRun, 'failures': len(result.failures), 'errors': len(result.errors)}, indent=2) + '\n')
    raise SystemExit(0 if result.wasSuccessful() else 1)

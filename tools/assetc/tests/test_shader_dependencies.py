"""Check incremental shader builds with real assetc/shaderc (Python stdlib only).

Example on Windows:
    python tools/assetc/tests/test_shader_dependencies.py \
        --assetc tutorials/hg_lua/harfang/assetc/assetc.exe \
        --toolchain tutorials/hg_lua/harfang/assetc/toolchains/host-windows-x64-target-windows-x64
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[3]
COMPUTE = {f"core/gaussian_splatting/{name}.sc"
           for name in ("clear", "gather", "project", "sort", "sort_local", "radix_histogram", "radix_prefix", "radix_scatter")}
SHADERS = COMPUTE | {f"{name}.{stage}" for name in (
    "core/gaussian_splatting/splat", "gaussian_splatting/shaders/gizmo")
    for stage in ("vsb", "fsb")}


class ShaderDependencyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="harfang shader dependencies ")
        self.addCleanup(self.temp.cleanup)
        self.source = Path(self.temp.name) / "resources"
        self.output = Path(self.temp.name) / "compiled"
        shutil.copytree(REPO / "core-package/core/gaussian_splatting",
                        self.source / "core/gaussian_splatting")
        for path in (REPO / "core-package/core/shader").glob("*.sh"):
            target = self.source / "core/shader" / path.name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
        for path in (REPO / "tutorials/resources/gaussian_splatting/shaders").glob("gizmo_*"):
            target = self.source / "gaussian_splatting/shaders" / path.name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)

    def compile(self, *flags):
        result = subprocess.run([
            str(OPTIONS.assetc), "-api", OPTIONS.api, "-platform", OPTIONS.platform,
            "-toolchain", str(OPTIONS.toolchain), *flags,
            str(self.source), str(self.output)],
            capture_output=True, text=True, errors="replace", timeout=60)
        log = result.stdout + result.stderr
        self.assertEqual(result.returncode, 0, log)
        self.assertRegex(log, r"\b0 failed\b")
        return log

    def timestamps(self):
        return {name: (self.output / name).stat().st_mtime_ns for name in SHADERS}

    def assert_noop(self, previous, *flags):
        log = self.compile(*flags)
        self.assertRegex(log, r"\b0 processed\b")
        self.assertIn("Removed 0 outputs due to missing input", log)
        self.assertNotIn("Spawning compile process", log)
        self.assertEqual(self.timestamps(), previous)

    def check_incremental(self, *flags):
        cold = self.compile(*flags)
        self.assertEqual(cold.count("Spawning compile process"), len(SHADERS), cold)
        before = self.timestamps()
        for _ in range(2):
            self.assert_noop(before, *flags)  # Each invocation reloads assetc.cab.

        # Only compute stages depend on this header, via ../shader/.
        with (self.source / "core/shader/bgfx_compute.sh").open("ab") as stream:
            stream.write(b"\n// Incremental compute dependency edit.\n")
        log = self.compile(*flags)
        compiled = re.findall(r"Spawning compile process for (.+) \(Compute Shader\)", log)
        self.assertEqual(set(compiled), COMPUTE, log)
        self.assertEqual(log.count("Spawning compile process"), len(COMPUTE), log)
        after = self.timestamps()
        self.assertEqual({name for name in SHADERS if after[name] != before[name]}, COMPUTE)
        self.assert_noop(after, *flags)

        # This is a transitive dependency of compute shaders and is shared with
        # the draw shaders through both ../ and ../../ include paths.
        with (self.source / "core/shader/bgfx_shader.sh").open("ab") as stream:
            stream.write(b"\n// Incremental shared dependency edit.\n")
        log = self.compile(*flags)
        self.assertEqual(log.count("Spawning compile process"), len(SHADERS), log)
        self.assert_noop(self.timestamps(), *flags)

        # Missing outputs must still rebuild individually.
        (self.output / "core/gaussian_splatting/clear.sc").unlink()
        log = self.compile(*flags)
        self.assertEqual(log.count("Spawning compile process"), 1, log)
        self.assertIn("core/gaussian_splatting/clear.sc (Compute Shader)", log)
        self.assert_noop(self.timestamps(), *flags)

    def test_content_hash_cache(self):
        self.check_incremental()

    def test_fast_check_cache(self):
        self.check_incremental("-fast_check")

    def test_guarded_include_cycle_with_path_aliases(self):
        # An alias must resolve to the same visited dependency before recursing.
        # The compiler's include guard makes this valid shader source.
        (self.source / "core/shader/cache_guard.sh").write_text(
            '#ifndef ASSETC_CACHE_GUARD\n#define ASSETC_CACHE_GUARD\n'
            '#include "../shader/./cache_guard.sh"\n#endif\n', encoding="utf-8")
        with (self.source / "core/shader/bgfx_compute.sh").open("ab") as stream:
            stream.write(b'\n#include "./cache_guard.sh"\n')
        self.compile()
        self.assert_noop(self.timestamps())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assetc", required=True, type=Path)
    parser.add_argument("--toolchain", required=True, type=Path)
    parser.add_argument("--api", default="DX11")
    parser.add_argument("--platform", default="windows")
    OPTIONS, remaining = parser.parse_known_args()
    OPTIONS.assetc = OPTIONS.assetc.resolve()
    OPTIONS.toolchain = OPTIONS.toolchain.resolve()
    unittest.main(argv=[__file__, *remaining])

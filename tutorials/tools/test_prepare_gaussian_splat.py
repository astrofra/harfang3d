#!/usr/bin/env python3
"""Unit tests for prepare_gaussian_splat.py (standard library only).

The fixtures are small, hand-written .splat files whose expected transform
results are stated explicitly. Tests cover known transform math, header and
payload layout, determinism, preset selection, and rejection of invalid input
before any output artifact is written.

Run with::

    python -m unittest discover -s tutorials/tools -p "test_prepare_gaussian_splat.py"
"""

from __future__ import annotations

import contextlib
import hashlib
import io
import math
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import prepare_gaussian_splat as splat  # noqa: E402
from prepare_gaussian_splat import (  # noqa: E402
    ConversionError,
    HEADER_STRUCT,
    PAYLOAD_STRUCT,
    convert,
    select_indices,
)

RECORD_STRUCT = struct.Struct("<6f8B")

# Known fixture bounds: min (-2, 0, 1), max (2, 4, 3).
# center = (0, 2, 2), height = 4, factor = 0.5, source up is -Y.
KNOWN_RECORDS = (
    # position, scale, RGBA, quaternion WXYZ bytes
    RECORD_STRUCT.pack(-2.0, 0.0, 1.0, 0.5, 0.25, 1.0, 255, 128, 0, 255, 192, 192, 128, 64),
    RECORD_STRUCT.pack(2.0, 4.0, 3.0, 2.0, 1.0, 0.5, 0, 255, 64, 0, 128, 255, 128, 128),
    RECORD_STRUCT.pack(0.0, 2.0, 2.0, 1.0, 1.0, 1.0, 10, 20, 30, 40, 128, 128, 192, 192),
)
KNOWN_SOURCE = b"".join(KNOWN_RECORDS)


def make_record(position, scale, rgba, quaternion):
    return RECORD_STRUCT.pack(*position, *scale, *rgba, *quaternion)


VALID_A = make_record((-1.0, -1.0, 0.0), (1.0, 1.0, 1.0), (255, 255, 255, 255), (192, 192, 128, 64))
VALID_B = make_record((1.0, 1.0, 0.0), (1.0, 1.0, 1.0), (0, 0, 0, 0), (128, 255, 128, 128))


class KnownConversionTest(unittest.TestCase):
    def setUp(self):
        self._temporary = tempfile.TemporaryDirectory()
        self.root = Path(self._temporary.name)
        self.source = self.root / "known.splat"
        self.source.write_bytes(KNOWN_SOURCE)
        self.out = self.root / "converted"
        self.manifest = convert(self.source, self.out)

    def tearDown(self):
        self._temporary.cleanup()

    def test_header_fields_and_transformed_bounds(self):
        blob = (self.out / "full.bin").read_bytes()
        self.assertEqual(len(blob), 64 + 3 * 64)
        header = HEADER_STRUCT.unpack_from(blob, 0)
        self.assertEqual(header[0], b"HGS0")
        self.assertEqual(header[1], 1)
        self.assertEqual(header[2], 3)
        self.assertEqual(header[3], 64)
        expected = (-1.0, -1.0, -0.5, 1.0, 1.0, 0.5)
        for actual, wanted in zip(header[4:10], expected):
            self.assertAlmostEqual(actual, wanted, places=6)
        self.assertEqual(header[10:16], (0, 0, 0, 0, 0, 0))

    def test_known_payload_math(self):
        blob = (self.out / "full.bin").read_bytes()

        first = PAYLOAD_STRUCT.unpack_from(blob, 64)
        self.assertAlmostEqual(first[0], -1.0, places=6)
        self.assertAlmostEqual(first[1], 1.0, places=6)
        self.assertAlmostEqual(first[2], 0.5, places=6)
        self.assertAlmostEqual(first[3], 1.0, places=6)
        self.assertAlmostEqual(first[4], 0.25, places=6)
        self.assertAlmostEqual(first[5], 0.125, places=6)
        self.assertAlmostEqual(first[6], 0.5, places=6)
        self.assertEqual(first[7], 0.0)
        # (0.5, 0.5, 0.0, -0.5) normalized, then left-multiplied by X-180.
        self.assertAlmostEqual(first[8], -0.5773503, places=6)
        self.assertAlmostEqual(first[9], 0.5773503, places=6)
        self.assertAlmostEqual(first[10], 0.5773503, places=6)
        self.assertAlmostEqual(first[11], 0.0, places=6)
        self.assertAlmostEqual(first[12], 1.0, places=6)
        self.assertAlmostEqual(first[13], 128.0 / 255.0, places=6)
        self.assertAlmostEqual(first[14], 0.0, places=6)
        self.assertEqual(first[15], 0.0)

        second = PAYLOAD_STRUCT.unpack_from(blob, 128)
        self.assertAlmostEqual(second[0], 1.0, places=6)
        self.assertAlmostEqual(second[1], -1.0, places=6)
        self.assertAlmostEqual(second[2], -0.5, places=6)
        self.assertAlmostEqual(second[3], 0.0, places=6)
        self.assertAlmostEqual(second[4], 1.0, places=6)
        self.assertAlmostEqual(second[5], 0.5, places=6)
        self.assertAlmostEqual(second[6], 0.25, places=6)
        self.assertAlmostEqual(second[8], -1.0, places=6)
        self.assertAlmostEqual(second[9], 0.0, places=6)
        self.assertAlmostEqual(second[10], 0.0, places=6)
        self.assertAlmostEqual(second[11], 0.0, places=6)
        self.assertAlmostEqual(second[12], 0.0, places=6)
        self.assertAlmostEqual(second[13], 1.0, places=6)
        self.assertAlmostEqual(second[14], 64.0 / 255.0, places=6)

        third = PAYLOAD_STRUCT.unpack_from(blob, 192)
        self.assertAlmostEqual(third[0], 0.0, places=6)
        self.assertAlmostEqual(third[1], 0.0, places=6)
        self.assertAlmostEqual(third[2], 0.0, places=6)
        self.assertAlmostEqual(third[3], 40.0 / 255.0, places=6)
        self.assertAlmostEqual(third[4], 0.5, places=6)
        self.assertAlmostEqual(third[5], 0.5, places=6)
        self.assertAlmostEqual(third[6], 0.5, places=6)
        self.assertAlmostEqual(third[8], 0.0, places=6)
        self.assertAlmostEqual(third[9], 0.0, places=6)
        self.assertAlmostEqual(third[10], -0.7071068, places=6)
        self.assertAlmostEqual(third[11], 0.7071068, places=6)
        self.assertAlmostEqual(third[12], 10.0 / 255.0, places=6)
        self.assertAlmostEqual(third[13], 20.0 / 255.0, places=6)
        self.assertAlmostEqual(third[14], 30.0 / 255.0, places=6)

    def test_manifest_fields_and_checksums(self):
        manifest = self.manifest
        self.assertEqual(manifest["version"], 1)
        self.assertEqual(manifest["source_sha256"], hashlib.sha256(KNOWN_SOURCE).hexdigest())
        self.assertEqual(manifest["source_bytes"], len(KNOWN_SOURCE))
        self.assertEqual(manifest["source_count"], 3)
        self.assertEqual(manifest["sh_degree"], 0)
        self.assertEqual(manifest["source_up_axis"], "-Y")
        self.assertEqual(manifest["selection"], "morton10-midpoint-quantiles-v1")
        self.assertAlmostEqual(manifest["scale_factor"], 0.5, places=6)
        for actual, wanted in zip(manifest["source_center"], (0.0, 2.0, 2.0)):
            self.assertAlmostEqual(actual, wanted, places=6)
        for actual, wanted in zip(manifest["bounds"]["min"], (-1.0, -1.0, -0.5)):
            self.assertAlmostEqual(actual, wanted, places=6)
        for actual, wanted in zip(manifest["bounds"]["max"], (1.0, 1.0, 0.5)):
            self.assertAlmostEqual(actual, wanted, places=6)

        self.assertEqual(set(manifest["presets"]), {"full", "60k", "30k"})
        full_bytes = (self.out / "full.bin").read_bytes()
        self.assertEqual(manifest["presets"]["full"]["file"], "full.bin")
        self.assertEqual(manifest["presets"]["full"]["count"], 3)
        self.assertEqual(manifest["presets"]["full"]["bytes"], len(full_bytes))
        self.assertEqual(
            manifest["presets"]["full"]["sha256"], hashlib.sha256(full_bytes).hexdigest()
        )
        for name in ("60k", "30k"):
            entry = manifest["presets"][name]
            self.assertEqual(entry["file"], f"{name}.bin")
            self.assertEqual(entry["count"], 3)
            self.assertEqual(entry["bytes"], 64 + 3 * 64)
            self.assertEqual(entry["sha256"], manifest["presets"]["full"]["sha256"])
            self.assertEqual((self.out / entry["file"]).read_bytes(), full_bytes)

    def test_output_has_no_absolute_paths_or_timestamps(self):
        manifest_text = (self.out / "manifest.json").read_text(encoding="utf-8")
        metadata_text = (self.out / "metadata.lua").read_text(encoding="utf-8")
        self.assertNotIn(str(self.root), manifest_text)
        self.assertNotIn(str(self.root), metadata_text)
        self.assertNotIn("timestamp", manifest_text.lower())
        self.assertNotIn("timestamp", metadata_text.lower())

    def test_metadata_lua_table(self):
        text = (self.out / "metadata.lua").read_text(encoding="utf-8")
        self.assertIn("return {", text)
        self.assertTrue(text.rstrip().endswith("}"))
        self.assertIn("source_count = 3", text)
        self.assertIn("bounds_min = {-1.0, -1.0, -0.5}", text)
        self.assertIn("bounds_max = {1.0, 1.0, 0.5}", text)
        self.assertIn("source_center = {0.0, 2.0, 2.0}", text)
        self.assertIn('full = {file = "full.bin", count = 3, offset = 64}', text)
        self.assertIn('["60k"] = {file = "60k.bin", count = 3, offset = 64}', text)
        self.assertIn('["30k"] = {file = "30k.bin", count = 3, offset = 64}', text)

    def test_rerun_is_byte_identical(self):
        second = self.root / "second"
        convert(self.source, second)
        for name in ("full.bin", "60k.bin", "30k.bin", "manifest.json", "metadata.lua"):
            self.assertEqual(
                (self.out / name).read_bytes(),
                (second / name).read_bytes(),
                msg=f"{name} is not deterministic",
            )


class InvalidInputTest(unittest.TestCase):
    def setUp(self):
        self._temporary = tempfile.TemporaryDirectory()
        self.root = Path(self._temporary.name)

    def tearDown(self):
        self._temporary.cleanup()

    def _assert_rejected(self, name, payload):
        source = self.root / f"{name}.splat"
        source.write_bytes(payload)
        out = self.root / f"out_{name}"
        with self.assertRaises(ConversionError):
            convert(source, out)
        self.assertFalse(out.exists(), msg=f"{name} left output artifacts behind")

    def test_invalid_lengths_and_empty_source(self):
        valid = VALID_A + VALID_B
        self._assert_rejected("empty", b"")
        self._assert_rejected("truncated", valid[:-1])
        self._assert_rejected("extra_byte", valid + b"\x00")

    def test_nonfinite_and_nonpositive_scales(self):
        self._assert_rejected(
            "nan_position",
            make_record((math.nan, 0.0, 0.0), (1.0, 1.0, 1.0), (0, 0, 0, 255), (192, 192, 128, 64))
            + VALID_B,
        )
        self._assert_rejected(
            "inf_scale",
            make_record((0.0, 0.0, 0.0), (math.inf, 1.0, 1.0), (0, 0, 0, 255), (192, 192, 128, 64))
            + VALID_B,
        )
        self._assert_rejected(
            "zero_scale",
            make_record((0.0, 0.0, 0.0), (1.0, 0.0, 1.0), (0, 0, 0, 255), (192, 192, 128, 64))
            + VALID_B,
        )
        self._assert_rejected(
            "negative_scale",
            make_record((0.0, 0.0, 0.0), (1.0, -1.0, 1.0), (0, 0, 0, 255), (192, 192, 128, 64))
            + VALID_B,
        )

    def test_zero_quaternion(self):
        self._assert_rejected(
            "zero_quaternion",
            make_record((0.0, 0.0, 0.0), (1.0, 1.0, 1.0), (0, 0, 0, 255), (128, 128, 128, 128))
            + VALID_B,
        )

    def test_degenerate_bounds(self):
        self._assert_rejected(
            "degenerate_bounds",
            make_record((-1.0, 0.0, 0.0), (1.0, 1.0, 1.0), (0, 0, 0, 255), (192, 192, 128, 64))
            + make_record((1.0, 0.0, 0.0), (1.0, 1.0, 1.0), (0, 0, 0, 255), (192, 192, 128, 64)),
        )

    def test_unrepresentable_float32_output(self):
        self._assert_rejected(
            "float32_overflow",
            make_record((-1e38, 0.0, 0.0), (1.0, 1.0, 1.0), (0, 0, 0, 255), (192, 192, 128, 64))
            + make_record((1e38, 1e-7, 0.0), (1.0, 1.0, 1.0), (0, 0, 0, 255), (192, 192, 128, 64)),
        )

    def test_hash_mismatch_and_malformed_hash(self):
        source = self.root / "hash.splat"
        source.write_bytes(VALID_A + VALID_B)
        for expected, name in (("0" * 64, "mismatch"), ("abc", "malformed"), ("z" * 64, "non_hex")):
            out = self.root / f"out_{name}"
            with self.assertRaises(ConversionError):
                convert(source, out, expected_sha256=expected)
            self.assertFalse(out.exists())

    def test_cli_returns_nonzero_and_writes_nothing(self):
        source = self.root / "bad.splat"
        source.write_bytes(b"not-a-splat")
        out = self.root / "cli_out"
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr):
            code = splat.main(["--source", str(source), "--out", str(out)])
        self.assertNotEqual(code, 0)
        self.assertIn("error", stderr.getvalue().lower())
        self.assertFalse(out.exists())


class CliSuccessTest(unittest.TestCase):
    def test_cli_accepts_uppercase_expected_hash(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "known.splat"
            source.write_bytes(KNOWN_SOURCE)
            out = root / "cli_out"
            digest = hashlib.sha256(KNOWN_SOURCE).hexdigest()
            code = splat.main(
                [
                    "--source",
                    str(source),
                    "--out",
                    str(out),
                    "--expected-sha256",
                    digest.upper(),
                ]
            )
            self.assertEqual(code, 0)
            for name in ("full.bin", "60k.bin", "30k.bin", "manifest.json", "metadata.lua"):
                self.assertTrue((out / name).is_file(), msg=f"missing {name}")


class PresetIntegrationTest(unittest.TestCase):
    def test_preset_header_counts_and_row_subset(self):
        # 30,001 records: the 30k preset must drop exactly one record while the
        # 60k preset stays identical to the full set. Kept just above the
        # smallest preset limit to avoid large fixtures.
        count = 30001
        records = []
        for index in range(count):
            records.append(
                make_record(
                    ((index % 173) * 0.01, (index // 173) * 0.01, 0.0),
                    (0.1, 0.2, 0.3),
                    (index % 256, (index * 3) % 256, (index * 7) % 256, 255),
                    (192, 192, 128, 64),
                )
            )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "large.splat"
            source.write_bytes(b"".join(records))
            out = root / "out"
            manifest = convert(source, out)

            full = (out / "full.bin").read_bytes()
            sixty = (out / "60k.bin").read_bytes()
            thirty = (out / "30k.bin").read_bytes()

            full_header = HEADER_STRUCT.unpack_from(full, 0)
            sixty_header = HEADER_STRUCT.unpack_from(sixty, 0)
            thirty_header = HEADER_STRUCT.unpack_from(thirty, 0)

            self.assertEqual(full_header[2], count)
            self.assertEqual(sixty_header[2], count)
            self.assertEqual(thirty_header[2], 30000)
            self.assertEqual(len(full), 64 + count * 64)
            self.assertEqual(len(sixty), 64 + count * 64)
            self.assertEqual(len(thirty), 64 + 30000 * 64)
            # Every binary reports the FULL transformed bounds.
            self.assertEqual(thirty_header[4:10], full_header[4:10])
            self.assertEqual(sixty_header[4:10], full_header[4:10])

            self.assertEqual(manifest["presets"]["full"]["count"], count)
            self.assertEqual(manifest["presets"]["60k"]["count"], count)
            self.assertEqual(manifest["presets"]["30k"]["count"], 30000)
            self.assertEqual(sixty, full)

            full_rows = {
                full[64 + 64 * index : 128 + 64 * index] for index in range(count)
            }
            thirty_rows = [
                thirty[64 + 64 * index : 128 + 64 * index] for index in range(30000)
            ]
            self.assertEqual(len(set(thirty_rows)), 30000)
            self.assertTrue(set(thirty_rows).issubset(full_rows))


class SelectIndicesTest(unittest.TestCase):
    @staticmethod
    def _grid():
        # 10x10 grid: x spans 0.0..4.5, y spans 0.0..4.5, z has zero extent.
        return [((index % 10) * 0.5, (index // 10) * 0.5, 0.0) for index in range(100)]

    def test_subset_is_unique_ascending_and_deterministic(self):
        positions = self._grid()
        picked = select_indices(positions, 15)
        self.assertEqual(len(picked), 15)
        self.assertEqual(len(set(picked)), 15)
        self.assertEqual(picked, sorted(picked))
        self.assertTrue(all(0 <= index < len(positions) for index in picked))
        self.assertEqual(picked, select_indices(positions, 15))
        self.assertEqual(select_indices(positions, 100), list(range(100)))
        self.assertEqual(select_indices(positions, 250), list(range(100)))

    def test_subset_preserves_spatial_coverage(self):
        positions = self._grid()
        picked = select_indices(positions, 15)
        xs = [positions[index][0] for index in picked]
        ys = [positions[index][1] for index in picked]
        self.assertLessEqual(min(xs), 1.0)
        self.assertGreaterEqual(max(xs), 4.0)
        self.assertLessEqual(min(ys), 1.0)
        self.assertGreaterEqual(max(ys), 4.0)

    def test_zero_extent_axes_are_supported(self):
        positions = [(float(index), 5.0, -2.0) for index in range(100)]
        picked = select_indices(positions, 15)
        self.assertEqual(len(picked), 15)
        self.assertEqual(len(set(picked)), 15)
        self.assertEqual(picked, sorted(picked))
        self.assertLessEqual(picked[0], 6)
        self.assertGreaterEqual(picked[-1], 93)

    def test_explicit_full_bounds_match_derived_bounds(self):
        positions = self._grid()
        bounds = ((0.0, 0.0, 0.0), (4.5, 4.5, 0.0))
        self.assertEqual(select_indices(positions, 15, bounds), select_indices(positions, 15))


if __name__ == "__main__":
    unittest.main()

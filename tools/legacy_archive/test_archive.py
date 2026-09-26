"""CLI round-trip, independent codec and malformed archive regression tests."""

import json
import lzma
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib


TOOL = str(Path(sys.argv.pop(1)).resolve())


class ArchiveTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / "source"
        self.source.mkdir()
        self.files = {
            "nested/text.bin": b"LZMA and Zlib archive tests\0" * 2048,
            "noise.bin": random.Random(42).randbytes(8192),
            "tiny.bin": b"123",
            "empty.bin": b"",
        }
        for name, data in self.files.items():
            path = self.source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)

    def run_tool(self, *args, success=True):
        result = subprocess.run([TOOL, *map(str, args)], capture_output=True, text=True)
        self.assertEqual(result.returncode == 0, success, result.stdout + result.stderr)
        return result

    def round_trip(self, *options):
        archive = self.root / "data.nac"
        self.run_tool("pack", "--allow-empty", *options, self.source, archive)
        self.run_tool("info", archive)
        entries = json.loads(self.run_tool("list", "--json", archive).stdout)["entries"]
        self.assertEqual({entry["alias"] for entry in entries}, set(self.files))
        self.run_tool("unpack", archive, self.root / "unpacked")
        raw = archive.read_bytes()
        for entry in entries:
            name = entry["alias"]
            self.assertEqual((self.root / "unpacked" / name).read_bytes(), self.files[name])
            offset, length = entry["offset"], entry["stored_length"]
            payload = raw[offset:offset + length]
            if entry["method"] == "LZMA":
                # Raw LZMA1 allows SDK dictionaries rounded to 4 KiB; the .lzma
                # container decoder imposes additional dictionary size restrictions.
                prop = payload[0]
                filters = [dict(id=lzma.FILTER_LZMA1, lc=prop % 9,
                                lp=(prop // 9) % 5, pb=prop // 45,
                                dict_size=int.from_bytes(payload[1:5], "little"))]
                decoded = lzma.decompress(payload[5:], format=lzma.FORMAT_RAW, filters=filters)
            elif entry["method"] == "Zlib":
                decoded = zlib.decompress(payload)
            else:
                decoded = payload
            self.assertEqual(decoded, self.files[name])
        return {entry["alias"]: entry["method"] for entry in entries}

    def test_default_lzma(self):
        methods = self.round_trip()
        self.assertEqual(methods["nested/text.bin"], "LZMA")
        self.assertEqual(methods["noise.bin"], "Raw")
        self.assertEqual(methods["tiny.bin"], "Raw")
        self.assertEqual(methods["empty.bin"], "Raw")

    def test_zlib(self):
        self.assertEqual(set(self.round_trip("--method", "zlib").values()), {"Zlib"})

    def test_raw(self):
        self.assertEqual(set(self.round_trip("-c", "-1").values()), {"Raw"})

    def test_legacy_header(self):
        self.round_trip("--legacy", "--method", "lzma", "-c", "9")

    def test_legacy_zlib(self):
        self.round_trip("--legacy", "--method", "zlib")

    def test_alignment_and_raw_pattern(self):
        methods = self.round_trip("--offset-padding", "16", "--size-padding", "512", "--raw", "nested/*")
        self.assertEqual(methods["nested/text.bin"], "Raw")
        self.assertEqual((self.root / "data.nac").stat().st_size % 512, 0)

    def test_level_zero(self):
        self.round_trip("--compression", "0")

    def test_bad_method(self):
        self.run_tool("pack", "--method", "unknown", self.source, self.root / "bad.nac", success=False)
        self.assertFalse((self.root / "bad.nac").exists())

    def test_external_lzma_and_corruption(self):
        content = self.files["nested/text.bin"]
        alone = lzma.compress(content, format=lzma.FORMAT_ALONE)
        payload = alone[:5] + alone[13:]
        archive = self.root / "external.nac"

        def write(method, data, size=len(content)):
            # Legacy header has no padding. One compressed entry, then EOF marker.
            archive.write_bytes(struct.pack("<II", 0x4E415243, 4) + b"file" +
                                struct.pack("<BII", method, size, len(data)) + data + b"\xff" * 4)

        write(2, payload)
        self.run_tool("unpack", archive, self.root / "valid")
        self.assertEqual((self.root / "valid/file").read_bytes(), content)
        for bad_payload, bad_size in [
            (b"\xff" + payload[1:], len(content)),
            (payload[:4], len(content)),
            (payload[:-1], len(content)),
            (payload + b"extra", len(content)),
            (payload, len(content) + 1),
            (payload, len(content) - 1),
        ]:
            with self.subTest(size=bad_size, payload_length=len(bad_payload)):
                write(2, bad_payload, bad_size)
                self.run_tool("unpack", "-f", archive, self.root / "invalid", success=False)
        for method in (3, 128, 255):
            write(method, payload)
            self.run_tool("list", archive, success=False)


if __name__ == "__main__":
    unittest.main()

"""Exercise the real texc/assetc binaries using generated images (Python stdlib only)."""

import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib


def write_png(path, width, height, rgba=(201, 73, 19, 127)):
    def chunk(kind, payload):
        return (struct.pack(">I", len(payload)) + kind + payload
                + struct.pack(">I", zlib.crc32(kind + payload)))

    pixels = (b"\0" + bytes(rgba) * width) * height
    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + chunk(b"IHDR", struct.pack(">2I5B", width, height, 8, 6, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(pixels)) + chunk(b"IEND", b""))


class TextureTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="harfang texc ")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.src = self.root / "input image.png"
        self.dst = self.root / "output image.dds"
        write_png(self.src, 8, 4)

    def run_tool(self, *args, success=True):
        result = subprocess.run([str(arg) for arg in args], capture_output=True,
                                text=True, errors="replace", timeout=60)
        if success:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout + result.stderr

    def check_dds(self, path, width, height, mip_count, dxgi):
        data = path.read_bytes()
        self.assertEqual(data[:4], b"DDS ")
        header = struct.unpack_from("<31I", data, 4)
        self.assertEqual(header[0], 124)
        self.assertEqual((header[3], header[2], header[6]), (width, height, mip_count))
        self.assertEqual(data[84:88], b"DX10")
        self.assertEqual(struct.unpack_from("<5I", data, 128), (dxgi, 3, 0, 1, 0))
        if mip_count > 1:
            self.assertEqual(header[26] & 0x400008, 0x400008)  # COMPLEX | MIPMAP
        raw = dxgi in (28, 87)
        if raw:
            self.assertTrue(header[1] & 8)  # DDSD_PITCH
            self.assertEqual(header[4], width * 4)
        else:
            self.assertTrue(header[1] & 0x80000)  # DDSD_LINEARSIZE
        expected_size = 148
        for _ in range(mip_count):
            expected_size += (width * height * 4 if raw else
                              ((width + 3) // 4) * ((height + 3) // 4)
                              * (8 if dxgi in (71, 80) else 16))
            width, height = max(1, width // 2), max(1, height // 2)
        self.assertEqual(len(data), expected_size)
        return data[148:]

    def test_formats_and_rectangular_mips(self):
        for fmt, dxgi in (("BC1", 71), ("BC3", 77), ("BC4", 80), ("BC5", 83),
                          ("BC7", 98), ("RGBA8", 28), ("BGRA8", 87)):
            with self.subTest(fmt=fmt):
                self.run_tool(OPTIONS.texc, "-f", fmt, "-m", self.src, self.dst)
                pixels = self.check_dds(self.dst, 8, 4, 4, dxgi)
                if fmt == "RGBA8":
                    self.assertEqual(pixels, bytes((201, 73, 19, 127)) * 43)
                if fmt == "BGRA8":
                    self.assertEqual(pixels, bytes((19, 73, 201, 127)) * 43)

    def test_npot_small_and_one_dimensional(self):
        for width, height in ((7, 3), (1, 8), (8, 1), (1, 1)):
            for fmt, dxgi in (("RGBA8", 28), ("BC7", 98)):
                with self.subTest(size=(width, height), fmt=fmt):
                    write_png(self.src, width, height)
                    self.run_tool(OPTIONS.texc, "-f", fmt, "-m", self.src, self.dst)
                    self.check_dds(self.dst, width, height, max(width, height).bit_length(), dxgi)

    def test_no_mips_and_alias(self):
        self.run_tool(OPTIONS.texc, "-f", "DXT5", self.src, self.dst)
        self.check_dds(self.dst, 8, 4, 1, 77)

    def test_resize_limits(self):
        for width, height, limit, expected in ((17, 7, 12, (8, 4)),
                                              (64, 1, 4, (4, 1)),
                                              (1, 64, 4, (1, 4))):
            with self.subTest(size=(width, height)):
                write_png(self.src, width, height)
                self.run_tool(OPTIONS.texc, "-f", "RGBA8", "-r", "-s", limit, "-m", self.src, self.dst)
                self.check_dds(self.dst, *expected, max(expected).bit_length(), 28)

    def test_quality_limits_and_errors(self):
        self.run_tool(OPTIONS.texc, "-f", "BC7", "-q", 10, self.src, self.dst)
        self.check_dds(self.dst, 8, 4, 1, 98)
        for args in (("-q", "-1"), ("-q", "11"), ("-q", "invalid"),
                     ("-s", "-1"), ("-f", "BC6H")):
            with self.subTest(args=args):
                self.run_tool(OPTIONS.texc, *args, self.src, self.dst, success=False)
        self.run_tool(OPTIONS.texc, self.root / "missing.png", self.dst, success=False)
        self.run_tool(OPTIONS.texc, self.src, self.root / "missing" / "output.dds", success=False)

    def test_bimg_can_decode_base_levels(self):
        if not OPTIONS.texturec:
            self.skipTest("pass --texturec to validate decoding with this checkout's bimg")
        for fmt in ("BC1", "BC3", "BC4", "BC5", "BC7", "RGBA8", "BGRA8"):
            with self.subTest(fmt=fmt):
                self.run_tool(OPTIONS.texc, "-f", fmt, "-m", self.src, self.dst)
                # Decode only mip 0: this bimg version overruns its decode buffer
                # below 4x4, including on DDS files generated by texturec itself.
                decoded = self.root / "decoded.dds"
                self.run_tool(OPTIONS.texturec, "-f", self.dst, "-o", decoded,
                              "--as", "dds", "-t", "RGBA8")
                data = decoded.read_bytes()
                self.assertEqual(data[:4], b"DDS ")
                offset = 148 if data[84:88] == b"DX10" else 128
                self.assertEqual(len(data) - offset, 8 * 4 * 4)
                # The legacy bimg BC4 decoder writes its single channel to B in BGRA.
                channel = 2 if fmt == "BC4" else 0
                self.assertLessEqual(abs(data[offset + channel] - 201), 8)

    def prepare_assetc(self, include_texc=True, include_texturec=False):
        if not OPTIONS.assetc:
            self.skipTest("pass --assetc to run integration tests")
        self.inputs = self.root / "resources with spaces"
        self.outputs = self.root / "compiled assets"
        self.toolchain = self.root / "toolchain with spaces"
        self.inputs.mkdir()
        self.toolchain.mkdir()
        for executable, include in ((OPTIONS.texc, include_texc), (OPTIONS.texturec, include_texturec)):
            if include:
                if not executable:
                    self.skipTest("pass --texturec to test fallback")
                shutil.copy2(executable, self.toolchain / executable.name)

    def add_resource(self, name, **meta):
        path = self.inputs / name
        write_png(path, 8, 4)
        path.with_name(path.name + ".meta").write_text(json.dumps({"profiles": {"default": meta}}))

    def compile_assets(self, api="GL"):
        output = self.run_tool(OPTIONS.assetc, self.inputs, self.outputs,
                               "-toolchain", self.toolchain, "-api", api, "-job", 2, "-verbose")
        self.assertNotIn("Skipping", output)
        self.assertNotIn("Failed", output)
        self.assertNotIn("CompileProcessReturnedNonZero", output)
        return output

    def test_assetc_texc_only_and_cache(self):
        self.prepare_assetc()
        self.add_resource("color.png", compression="RAW")
        self.add_resource("compressed.png", compression="BC7", **{"generate-mips": False, "max-size": 4})
        self.compile_assets()
        self.check_dds(self.outputs / "color.png", 8, 4, 4, 28)
        self.check_dds(self.outputs / "compressed.png", 4, 2, 1, 98)
        before = (self.outputs / "color.png").stat().st_mtime_ns
        cached = self.compile_assets()
        self.assertIn("Texture up to date", cached)
        self.assertNotIn("Spawning compile process", cached)
        self.assertEqual((self.outputs / "color.png").stat().st_mtime_ns, before)
        self.compile_assets("DX11")
        self.check_dds(self.outputs / "color.png", 8, 4, 4, 87)

    def test_assetc_fallback_and_encoder_cache_change(self):
        self.prepare_assetc(include_texc=False, include_texturec=True)
        self.add_resource("color.png", compression="RAW")
        self.compile_assets()
        shutil.copy2(OPTIONS.texc, self.toolchain / OPTIONS.texc.name)
        output = self.compile_assets()
        self.assertIn("Spawning compile process", output)
        self.check_dds(self.outputs / "color.png", 8, 4, 4, 28)
        (self.toolchain / OPTIONS.texc.name).unlink()
        self.assertIn("Spawning compile process", self.compile_assets())

    def test_assetc_unsupported_format_and_hdr_fallback(self):
        self.prepare_assetc(include_texturec=True)
        self.add_resource("bc2.png", compression="BC2")
        hdr = self.inputs / "light.hdr"
        hdr.write_bytes(b"#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 2 +X 2\n" + bytes((128, 64, 32, 129)) * 4)
        self.compile_assets()
        self.assertEqual((self.outputs / "bc2.png").read_bytes()[:4], b"DDS ")
        self.assertEqual((self.outputs / "light.hdr").read_bytes()[:4], b"DDS ")

    def test_assetc_texc_precedes_texconv(self):
        self.prepare_assetc(include_texturec=True)
        # An unusable texconv must never be launched for a texc-supported texture.
        suffix = OPTIONS.texc.suffix
        (self.toolchain / ("texconv" + suffix)).write_bytes(b"invalid executable")
        self.add_resource("color.png", compression="BC7")
        self.compile_assets()
        self.check_dds(self.outputs / "color.png", 8, 4, 4, 98)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--texc", type=Path, required=True)
    parser.add_argument("--assetc", type=Path)
    parser.add_argument("--texturec", type=Path)
    OPTIONS, remaining = parser.parse_known_args()
    for key, path in vars(OPTIONS).items():
        if path:
            setattr(OPTIONS, key, path.resolve(strict=True))
    unittest.main(argv=[sys.argv[0]] + remaining)

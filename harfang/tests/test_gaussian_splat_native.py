#!/usr/bin/env python3
"""Native HGS2 converter and optional Lua/GPU integration probe (stdlib only)."""

import argparse
import gzip
import zlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import gaussian_splat_hgs2 as codec


C0 = 0.28209479177387814
C1 = 0.4886025119029199
BASE_PROPERTIES = [
    "x", "y", "z", "f_dc_0", "f_dc_1", "f_dc_2", "opacity",
    "scale_0", "scale_1", "scale_2", "rot_0", "rot_1", "rot_2", "rot_3",
]
SENTINEL = b"pre-existing output must survive\x00\xff"


class TestFailure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise TestFailure(message)


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def require_float(actual, expected, label, tolerance=2e-6):
    require(math.isfinite(actual), f"{label}: non-finite output {actual}")
    require(math.isclose(actual, expected, rel_tol=1e-6, abs_tol=tolerance),
            f"{label}: got {actual}, expected {expected}")


def require_sequence(actual, expected, label, tolerance=2e-6):
    require(len(actual) == len(expected), f"{label}: length {len(actual)} != {len(expected)}")
    for index, (got, want) in enumerate(zip(actual, expected)):
        require_float(got, want, f"{label}[{index}]", tolerance)


def properties_for_degree(degree):
    rest_count = 3 * ((degree + 1) ** 2 - 1)
    return BASE_PROPERTIES + [f"f_rest_{i}" for i in range(rest_count)]


def make_record(index, degree=3):
    non_dc = (degree + 1) ** 2 - 1
    record = {
        "x": float(index - 3), "y": 0.25 * index, "z": 2.0 * index,
        "f_dc_0": 0.125 * index - 0.3, "f_dc_1": -0.0625 * index,
        "f_dc_2": 0.03125 * index + 0.1, "opacity": -1.0 + 0.25 * index,
        "scale_0": -1.0 + 0.1 * index, "scale_1": -0.5 - 0.05 * index,
        "scale_2": -0.75 + 0.02 * index,
        "rot_0": 1.0 + index, "rot_1": 0.5 * index,
        "rot_2": -0.25 * index, "rot_3": 0.125 * index,
    }
    if index == 3:
        record.update(rot_0=1.0, rot_1=0.0, rot_2=0.0, rot_3=0.0)
    for channel in range(3):
        for coefficient in range(non_dc):
            name = f"f_rest_{channel * non_dc + coefficient}"
            record[name] = 0.1 * (index + 1) + 0.03 * channel + 0.005 * coefficient
    return record


def ply_bytes(records, degree=3, properties=None, ascii_format=False,
              type_overrides=None, extra_element=False):
    properties = list(properties_for_degree(degree) if properties is None else properties)
    type_overrides = type_overrides or {}
    lines = ["ply", f"format {'ascii' if ascii_format else 'binary_little_endian'} 1.0",
             f"element vertex {len(records)}"]
    for name in properties:
        lines.append(f"property {type_overrides.get(name, 'float')} {name}")
    if extra_element:
        lines += ["element face 1", "property list uchar int vertex_indices"]
    lines.append("end_header")
    header = ("\n".join(lines) + "\n").encode("ascii")
    if ascii_format:
        body = "".join(" ".join(str(record[name]) for name in properties) + "\n" for record in records)
        if extra_element:
            body += "3 0 0 0\n"
        return header + body.encode("ascii")
    formats = {"float": "f", "double": "d", "int": "i", "uchar": "B"}
    payload = bytearray()
    for record in records:
        for name in properties:
            kind = type_overrides.get(name, "float")
            value = record.get(name, 0.0)  # Deliberately extra malformed-schema properties.
            if kind in ("int", "uchar"):
                value = int(value)
            payload += struct.pack("<" + formats[kind], value)
    if extra_element:
        payload += struct.pack("<Biii", 3, 0, 0, 0)
    return header + bytes(payload)


def write_ply(path, records, **kwargs):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(ply_bytes(records, **kwargs))


def read_hgs(path):
    try:
        return codec.read_hgs(path)
    except (ValueError, struct.error, zlib.error) as error:
        raise TestFailure(f'{path}: {error}') from error


def require_source_record(header, output_index, source, source_non_dc):
    base = header["base"][output_index]
    require_sequence(base[0:3], [f32(source[name]) for name in ("x", "y", "z")],
                     f"record {output_index} position")
    logit = f32(source["opacity"])
    require_float(base[3], 1.0 / (1.0 + math.exp(-logit)), f"record {output_index} opacity")
    require_sequence(base[4:7], [math.exp(f32(source[f"scale_{axis}"])) for axis in range(3)],
                     f"record {output_index} scale")
    quaternion = [f32(source[f"rot_{axis}"]) for axis in range(4)]
    norm = math.sqrt(sum(value * value for value in quaternion))
    require_sequence(base[8:12], [value / norm for value in quaternion],
                     f"record {output_index} quaternion")
    require_sequence(base[12:15], [f32(source[f"f_dc_{axis}"]) for axis in range(3)],
                     f"record {output_index} DC")
    for coefficient, actual in enumerate(header["higher"][output_index]):
        expected = [f32(source[f"f_rest_{channel * source_non_dc + coefficient}"])
                    for channel in range(3)] + [0.0]
        require_sequence(actual, expected, f"record {output_index} SH coefficient {coefficient}")


class Harness:
    def __init__(self, args):
        self.gsplatc = args.gsplatc.resolve()
        self.lua = args.lua.resolve() if args.lua else None
        self.module = args.module.resolve() if args.module else None
        self.shaders = args.shaders.resolve() if args.shaders else None
        self.mesh_shaders = args.mesh_shaders.resolve() if args.mesh_shaders else self.shaders
        self.renderer = args.renderer
        self.output = args.output.resolve()
        self.output.mkdir(parents=True, exist_ok=True)
        (self.output / "logs").mkdir(exist_ok=True)
        self.commands = []

    @property
    def gpu_enabled(self):
        return self.lua is not None

    def run(self, label, command, expected=0, marker=None, timeout=45):
        command = [str(part) for part in command]
        log_path = self.output / "logs" / f"{label}.log"
        entry = {"label": label, "command": command, "expected": expected,
                 "log": str(log_path.relative_to(self.output))}
        self.commands.append(entry)
        try:
            result = subprocess.run(command, cwd=self.output, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    timeout=timeout)
            output = result.stdout
            entry["exit_code"] = result.returncode
        except subprocess.TimeoutExpired as error:
            output = error.stdout or ""
            if isinstance(output, bytes):
                output = output.decode("utf-8", errors="replace")
            output += "\nTIMEOUT: process terminated\n"
            entry["timeout"] = True
            log_path.write_text(output, encoding="utf-8")
            raise TestFailure(f"{label}: timed out after {timeout}s; see {log_path}") from error
        except OSError as error:
            entry["launch_error"] = str(error)
            log_path.write_text(str(error), encoding="utf-8")
            raise TestFailure(f"{label}: could not launch process: {error}") from error
        log_path.write_text(output, encoding="utf-8")
        if expected == "nonzero":
            require(result.returncode != 0, f"{label}: invalid input unexpectedly succeeded; see {log_path}")
        else:
            require(result.returncode == expected,
                    f"{label}: exit {result.returncode}, expected {expected}; see {log_path}")
        if marker is not None:
            require(marker in output, f"{label}: missing required marker {marker!r}; see {log_path}")
        return output

    def convert(self, label, source, destination, *options, expected=0):
        destination.parent.mkdir(parents=True, exist_ok=True)
        # Importer tests use the lossless representation of HGS2 to isolate import accuracy.
        if '--storage' not in options:
            options = (*options, '--storage', 'float32')
        return self.run(label, [self.gsplatc, source, destination, *options], expected=expected)


def spz_payload(version=3, degree=3, count=7):
    """Small independently packed SPZ fixture with signed positions and all SH channels."""
    header = struct.pack('<III4B', 0x5053474e, version, count, degree, 12, 1, 0)
    positions = b''.join(int(value * 4096).to_bytes(3, 'little', signed=True)
                         for i in range(count) for value in (i - 3, i / 4, i * 2))
    alpha = bytes(0 if i == 1 else 255 if i == 3 else 128 for i in range(count))
    colors = bytes(v for i in range(count) for v in (32 + i, 128, 224 - i))
    scales = bytes([128, 144, 160]) * count
    if version == 2:
        rotations = bytes([153, 102, 128]) * count
    else:
        # Slots in descending XYZW index order: +100, -200, +50 (signed magnitude).
        rotations = b''.join(struct.pack('<I', ((i % 4) << 30) | 100 | ((512 + 200) << 10) | (50 << 20))
                             for i in range(count))
    sh = bytes((i * 7 + k * 3 + c * 41) % 256 for i in range(count)
               for k in range((degree + 1) ** 2 - 1) for c in range(3))
    return header + positions + alpha + colors + scales + rotations + sh


def spz_bytes(version=3, degree=3, count=7):
    return gzip.compress(spz_payload(version, degree, count), mtime=0)


def repeated_spz_bytes(count, degree=0):
    """A valid, cheaply generated large fixture; no per-point Python objects."""
    non_dc = (degree + 1) ** 2 - 1
    payload = (struct.pack('<III4B', 0x5053474e, 3, count, degree, 12, 0, 0)
               + bytes(9 * count) + bytes([255]) * count
               + bytes([128]) * (3 * count) + bytes([144]) * (3 * count)
               + struct.pack('<I', 3 << 30) * count + bytes([128]) * (3 * non_dc * count))
    return gzip.compress(payload, mtime=0)


def test_full_count_default(h):
    root = h.output / 'full_count'
    root.mkdir(exist_ok=True)
    count = 250001  # Cross the old implicit reduction threshold.
    one_ply = ply_bytes([make_record(0, degree=0)], degree=0)
    header_end = one_ply.index(b'end_header\n') + len(b'end_header\n')
    fixtures = {
        'splat': splat_bytes(1) * count,
        'ply': one_ply[:header_end].replace(b'element vertex 1\n',
                                          f'element vertex {count}\n'.encode())
               + one_ply[header_end:] * count,
        'spz': repeated_spz_bytes(count),
    }
    for extension, payload in fixtures.items():
        source, destination = root / ('full.' + extension), root / (extension + '.hgs')
        source.write_bytes(payload)
        h.convert('full_default_' + extension, source, destination, '--storage', 'quantized')
        with destination.open('rb') as stream:
            header = stream.read(128)
        require(header[:4] == b'HGS2' and struct.unpack_from('<II', header, 32) == (count, count),
                extension + ': omitted max-count silently reduced the source')
        h.convert('full_explicit_cap_' + extension, source, destination, '--max-count', '3')
        data = read_hgs(destination)
        require((data['splat_count'], data['source_count']) == (3, count),
                extension + ': explicit reduction lost source count')

    # Retain more than the old native hard limit as well as the old default cap.
    source, destination = root / 'million.splat', root / 'million.hgs'
    source.write_bytes(splat_bytes(1) * 1048577)
    h.convert('full_above_old_native_limit', source, destination, '--storage', 'quantized')
    with destination.open('rb') as stream:
        header = stream.read(128)
    require(struct.unpack_from('<II', header, 32) == (1048577, 1048577), 'Old native count limit remains')

    # Unsupported full counts must fail, never be clamped to the hard limit.
    source, destination = root / 'over-limit.splat', root / 'over-limit.hgs'
    source.write_bytes(splat_bytes(1) * (2097152 + 1))
    destination.write_bytes(SENTINEL)
    h.convert('full_count_limit', source, destination, expected='nonzero')
    require(destination.read_bytes() == SENTINEL, 'Failed full import overwrote its destination')
    h.convert('full_count_limit_explicit_reduction', source, destination, '--max-count', '3')
    require(read_hgs(destination)['source_count'] == 2097153, 'Explicit reduction of a larger source failed')
    for cap in ('0', '2097153'):
        destination.write_bytes(SENTINEL)
        h.convert('full_invalid_cap_' + cap, source, destination, '--max-count', cap, expected='nonzero')
        require(destination.read_bytes() == SENTINEL, 'Invalid explicit cap overwrote its destination')

    # A legal count may still exceed the expanded SH3 byte budget. Fail before
    # allocating the full canonical arrays, even when quantized output is small.
    source = root / 'over-budget.spz'
    source.write_bytes(repeated_spz_bytes(2097152, degree=3))
    destination.write_bytes(SENTINEL)
    log = h.convert('full_expanded_byte_budget', source, destination, '--storage', 'quantized', expected='nonzero')
    require('512 MiB' in log, 'Expanded byte-budget rejection was not diagnosed')
    require(destination.read_bytes() == SENTINEL, 'Byte-budget failure overwrote its destination')


def test_spz_values(h):
    root = h.output / 'cpu'
    root.mkdir(exist_ok=True)
    source, destination = root / 'fixture.SPZ', root / 'spz.hgs'
    for version in (2, 3):
        source.write_bytes(spz_bytes(version))
        h.convert(f'spz_v{version}', source, destination, '--max-count', '7')
        data = read_hgs(destination)
        require((data['source_count'], data['splat_count'], data['sh_degree'], data['flags']) == (7, 7, 3, 5),
                'SPZ metadata/AA preservation')
        for i, base in enumerate(data['base']):
            require_sequence(base[:3], (i - 3, i / 4, i * 2), 'SPZ signed fixed-point position')
            require_sequence(base[4:7], (math.exp(-2), math.exp(-1), 1), 'SPZ logarithmic scales')
            require_float(base[3], 0 if i == 1 else 1 if i == 3 else 128 / 255, 'SPZ alpha')
            require_sequence(base[12:15], [(v / 255 - .5) / .15 for v in (32 + i, 128, 224 - i)], 'SPZ DC')
            if version == 2:
                xyz = [v / 127.5 - 1 for v in (153, 102, 128)]
                q = [math.sqrt(1 - sum(v * v for v in xyz))] + xyz
            else:
                # Independent explicit placement, not an implementation bit-unpacking oracle.
                a, b, c = [v / (511 * math.sqrt(2)) for v in (50, -200, 100)]
                largest = math.sqrt(1 - a*a - b*b - c*c)
                xyzw = list((a, b, c))
                xyzw.insert(i % 4, largest)
                q = [xyzw[3], *xyzw[:3]]
            require_sequence(base[8:12], q, 'SPZ quaternion WXYZ')
            for k, coefficients in enumerate(data['higher'][i]):
                require_sequence(coefficients, [((i * 7 + k * 3 + c * 41) % 256 - 128) / 128
                                                for c in range(3)] + [0], 'SPZ coefficient-major RGB')
        for degree in range(4):
            h.convert(f'spz_v{version}_cap{degree}', source, destination, '--max-count', '3',
                      '--sh-degree', str(degree), '--normalize')
            reduced = read_hgs(destination)
            require((reduced['splat_count'], reduced['sh_degree']) == (3, degree), 'SPZ reduction metadata')
            require(reduced['flags'] & 3 == 3, 'SPZ normalization/AA flags')
            for j, i in enumerate((1, 3, 5)):
                original = data['base'][i]
                require_sequence(reduced['base'][j][:3], [(original[k] - (0, .75, 6)[k]) / 6 for k in range(3)],
                                 'SPZ normalize full-source bounds')
                require_sequence(reduced['base'][j][4:7], [v / 6 for v in original[4:7]], 'SPZ normalized scales')
                require(reduced['higher'][j] == data['higher'][i][:(degree + 1)**2 - 1], 'SPZ truncated SH stride')
    source.write_bytes(spz_bytes(degree=0))
    h.convert('spz_no_invented_bands', source, destination)
    require(read_hgs(destination)['sh_degree'] == 0, 'SPZ invented missing bands')
    # Valid optional gzip fields including header CRC16 must be accepted.
    compressed = spz_bytes()
    header = bytes([31,139,8,30,0,0,0,0,0,255]) + struct.pack('<H',3) + b'xyz' + b'name.spz\0comment\0'
    header += struct.pack('<H', zlib.crc32(header) & 65535)
    source.write_bytes(header + compressed[10:])
    h.convert('spz_gzip_optional_fields', source, destination)


def test_spz_rejections(h):
    root = h.output / 'cpu'
    source, destination = root / 'invalid.spz', root / 'invalid-spz.hgs'
    raw = spz_payload()
    compressed = spz_bytes()
    invalid = {'empty': b'', 'truncated_gzip': compressed[:-5], 'trailing_gzip': compressed + b'x',
               'concatenated_gzip': compressed + compressed, 'not_gzip': raw,
               'raw_v4': struct.pack('<II', 0x5053474e, 4) + bytes(24),
               'short_header': gzip.compress(raw[:15]), 'short_payload': gzip.compress(raw[:-1]),
               'trailing_payload': gzip.compress(raw + b'x')}
    for name, offset, fmt, value in [('magic',0,'<I',0),('version1',4,'<I',1),('version4',4,'<I',4),
                                    ('count0',8,'<I',0),('huge_count',8,'<I',0xffffffff),
                                    ('degree4',12,'<B',4),('fractional',13,'<B',255),
                                    ('extensions',14,'<B',2),('flags',14,'<B',128),('reserved',15,'<B',1)]:
        bad = bytearray(raw)
        struct.pack_into(fmt, bad, offset, value)
        invalid[name] = gzip.compress(bad)
    # Invalid quaternion belongs to source index 0, discarded by midpoint selection.
    bad = bytearray(raw)
    struct.pack_into('<I', bad, 16 + 7 * (9 + 1 + 3 + 3), 0x1fffffff)
    invalid['dropped_quaternion'] = gzip.compress(bad)
    for name, offset, value in [('crc',-8,1),('size',-4,1),('deflate',10,255),('gzip_flags',3,224),('method',2,0)]:
        bad = bytearray(compressed)
        bad[offset] ^= value
        if name == 'method': bad[offset] = 0
        invalid[name] = bad
    header = bytes([31,139,8,2,0,0,0,0,0,255])
    invalid['header_crc'] = header + struct.pack('<H', (zlib.crc32(header) ^ 1) & 65535) + compressed[10:]
    invalid['unterminated_name'] = bytes([31,139,8,8,0,0,0,0,0,255]) + b'A' * 30 + bytes(8)
    bad = bytearray(compressed)
    struct.pack_into('<I', bad, len(bad) - 4, 0x7fffffff)
    invalid['expansion_budget'] = bad
    for name, payload in invalid.items():
        source.write_bytes(payload)
        destination.write_bytes(SENTINEL)
        h.convert('spz_reject_' + name, source, destination, '--max-count', '3', expected='nonzero')
        require(destination.read_bytes() == SENTINEL, 'Invalid SPZ overwrote destination')


def splat_bytes(count=7):
    return b''.join(struct.pack('<6f8B', i - 3, i * .25, i * 2, .25, .5, 1,
                                255, 64, 0, 0 if i == 1 else 128, 255, 160, 128, 96)
                    for i in range(count))


def test_binary_splat(h):
    root = h.output / "cpu"
    root.mkdir(exist_ok=True)
    source, destination = root / "plant.SPLAT", root / "binary.hgs"
    source.write_bytes(splat_bytes())
    h.convert("splat_values", source, destination, "--max-count", "3", "--sh-degree", "3")
    data = read_hgs(destination)
    require((data['splat_count'], data['source_count'], data['sh_degree'], data['flags']) == (3, 7, 0, 0),
            "binary counts, SH0 or flags")
    for output, index in enumerate((1, 3, 5)):
        base = data['base'][output]
        require_sequence(base[:3], (index - 3, index * .25, index * 2), "binary midpoint")
        require_sequence(base[4:7], (.25, .5, 1), "linear scales")
        require_float(base[3], 0 if index == 1 else 128 / 255, "linear opacity")
        require_sequence([.5 + C0 * value for value in base[12:15]], (1, 64 / 255, 0), "SH0 RGB")
        norm = math.sqrt(127 ** 2 + 32 ** 2 + 32 ** 2)
        require_sequence(base[8:12], [127 / norm, 32 / norm, 0, -32 / norm], "packed WXYZ")
    h.convert("splat_normalized", source, destination, "--max-count", "3", "--normalize")
    normalized = read_hgs(destination)
    for index, base in enumerate(normalized['base']):
        original = data['base'][index]
        require_sequence(base[:3], [(original[j] - (0, .75, 6)[j]) / 6 for j in range(3)],
                         "full source normalization")
        require_sequence(base[4:7], [v / 6 for v in original[4:7]], "normalized linear scales")
    invalid = {'empty': b'', 'partial': splat_bytes() + b'x'}
    for name, offset, fmt, value in [('nan_dropped', 0, '<f', float('nan')),
                                    ('scale_dropped', 12, '<f', 0),
                                    ('quat_dropped', 28, '<I', 0x80808080)]:
        payload = bytearray(splat_bytes())
        struct.pack_into(fmt, payload, offset, value)
        invalid[name] = payload
    for name, payload in invalid.items():
        source.write_bytes(payload)
        destination.write_bytes(SENTINEL)
        h.convert('splat_' + name, source, destination, '--max-count', '3', expected='nonzero')
        require(destination.read_bytes() == SENTINEL, 'Invalid binary input overwrote destination')
    unknown = root / 'unknown.unsupported'
    unknown.write_bytes(splat_bytes())
    h.convert('splat_unknown_extension', unknown, destination, expected='nonzero')


def test_midpoint_selection_and_values(h):
    root = h.output / "cpu"
    records = [make_record(i) for i in range(7)]
    source = root / "seven.ply"
    write_ply(source, records)
    destination = root / "midpoint.hgs"
    h.convert("cpu_midpoint", source, destination, "--max-count", "3", "--sh-degree", "3")
    header = read_hgs(destination)
    require(header["splat_count"] == 3 and header["source_count"] == 7, "midpoint counts")
    require(header["flags"] == 4, "identity import must not set normalize or antialiasing flags")
    retained = [1, 3, 5]
    for output_index, source_index in enumerate(retained):
        require_source_record(header, output_index, records[source_index], 15)
    positions = [[f32(records[i][name]) for name in ("x", "y", "z")] for i in retained]
    expected_min = tuple(min(position[axis] for position in positions) for axis in range(3))
    expected_max = tuple(max(position[axis] for position in positions) for axis in range(3))
    require_sequence(header["bounds_min"], expected_min, "bounds min")
    require_sequence(header["bounds_max"], expected_max, "bounds max")


def test_sh_degrees_and_missing_bands(h):
    root = h.output / "cpu"
    records = [make_record(i) for i in range(7)]
    source = root / "degrees.ply"
    write_ply(source, records)
    base_bytes = None
    for degree in range(4):
        destination = root / f"degree_{degree}.hgs"
        h.convert(f"cpu_degree_{degree}", source, destination,
                  "--max-count", "3", "--sh-degree", str(degree))
        header = read_hgs(destination)
        require(header["sh_degree"] == degree, f"requested degree {degree}")
        require(header["non_dc_coefficient_count"] == (degree + 1) ** 2 - 1,
                f"degree {degree} non-DC count")
        if base_bytes is None:
            base_bytes = header["base_bytes"]
        else:
            require(header["base_bytes"] == base_bytes, f"degree {degree} changed base records")
        for output_index, source_index in enumerate((1, 3, 5)):
            for coefficient, actual in enumerate(header["higher"][output_index]):
                expected = [f32(records[source_index][f"f_rest_{channel * 15 + coefficient}"])
                            for channel in range(3)] + [0.0]
                require_sequence(actual, expected, f"degree {degree} channel mapping")
    degree_one_source = root / "degree_one_only.ply"
    write_ply(degree_one_source, [make_record(2, degree=1)], degree=1)
    destination = root / "degree_one_only.hgs"
    h.convert("cpu_missing_bands", degree_one_source, destination, "--sh-degree", "3")
    header = read_hgs(destination)
    require(header["sh_degree"] == 1 and len(header["higher"][0]) == 3,
            "importer invented missing SH bands")
    require_source_record(header, 0, make_record(2, degree=1), 3)


def test_explicit_normalization(h):
    root = h.output / "cpu"
    source = root / "normalize.ply"
    write_ply(source, [make_record(i) for i in range(7)])
    plain_path, normalized_path = root / "plain.hgs", root / "normalized.hgs"
    h.convert("cpu_normalize_plain", source, plain_path, "--max-count", "3")
    h.convert("cpu_normalize_explicit", source, normalized_path, "--max-count", "3", "--normalize")
    plain, normalized = read_hgs(plain_path), read_hgs(normalized_path)
    require(not plain["flags"] & 2 and normalized["flags"] & 2, "normalized flag")
    require(normalized["higher_bytes"] == plain["higher_bytes"], "normalization changed SH")
    source_records = [make_record(i) for i in range(7)]
    source_min = [min(record[axis] for record in source_records) for axis in ("x", "y", "z")]
    source_max = [max(record[axis] for record in source_records) for axis in ("x", "y", "z")]
    center = [(lo + hi) * 0.5 for lo, hi in zip(source_min, source_max)]
    extent = max(hi - lo for lo, hi in zip(source_min, source_max))
    require(extent > 0.0, "normalization fixture has empty bounds")
    factor = 2.0 / extent
    for index, (before, after) in enumerate(zip(plain["base"], normalized["base"])):
        require_sequence(after[0:3], [(before[axis] - center[axis]) * factor for axis in range(3)],
                         f"normalized position {index}")
        require_sequence(after[4:7], [before[axis] * factor for axis in range(4, 7)],
                         f"normalized scale {index}")
        for indices, label in ((range(3, 4), "opacity"), (range(8, 12), "quaternion"),
                               (range(12, 15), "DC")):
            require_sequence([after[i] for i in indices], [before[i] for i in indices],
                             f"normalized {label} {index}")
    expected_min = [(value - center[axis]) * factor for axis, value in enumerate(plain["bounds_min"])]
    expected_max = [(value - center[axis]) * factor for axis, value in enumerate(plain["bounds_max"])]
    require_sequence(normalized["bounds_min"], expected_min, "normalized bounds min")
    require_sequence(normalized["bounds_max"], expected_max, "normalized bounds max")


def test_reordered_required_properties(h):
    root = h.output / "cpu"
    records = [make_record(0, degree=2), make_record(1, degree=2)]
    canonical, reordered = root / "canonical.ply", root / "reordered.ply"
    write_ply(canonical, records, degree=2)
    write_ply(reordered, records, properties=reversed(properties_for_degree(2)))
    canonical_hgs, reordered_hgs = root / "canonical.hgs", root / "reordered.hgs"
    h.convert("cpu_canonical_order", canonical, canonical_hgs, "--sh-degree", "2")
    h.convert("cpu_reordered_order", reordered, reordered_hgs, "--sh-degree", "2")
    require(canonical_hgs.read_bytes() == reordered_hgs.read_bytes(),
            "property order changed native output")


def expect_import_failure(h, label, source, *options):
    root = h.output / "cpu" / "invalid"
    sentinel = root / f"{label}.sentinel.hgs"
    sentinel.parent.mkdir(parents=True, exist_ok=True)
    sentinel.write_bytes(SENTINEL)
    h.convert(f"invalid_{label}_sentinel", source, sentinel, *options, expected="nonzero")
    require(sentinel.read_bytes() == SENTINEL, f"{label}: overwrote sentinel on failure")
    absent = root / f"{label}.absent.hgs"
    if absent.exists():
        absent.unlink()
    h.convert(f"invalid_{label}_absent", source, absent, *options, expected="nonzero")
    require(not absent.exists(), f"{label}: left a half-created output file")


def test_invalid_ply_schema_and_payload(h):
    root = h.output / "cpu" / "invalid"
    valid_record = make_record(1, degree=1)
    properties = properties_for_degree(1)
    cases = {}

    def case(name, records=None, **kwargs):
        cases[name] = ply_bytes([valid_record] if records is None else records, degree=1, **kwargs)

    case("missing_y", properties=[name for name in properties if name != "y"])
    case("duplicate_x", properties=properties + ["x"])
    case("malformed_rest_short", properties=BASE_PROPERTIES + [f"f_rest_{i}" for i in range(8)])
    case("malformed_rest_channel", properties=BASE_PROPERTIES + [f"f_rest_{i}" for i in range(10)])
    case("ascii", ascii_format=True)
    case("unsupported_double", type_overrides={"x": "double"})
    case("extra_element", extra_element=True)
    bad_scale = dict(valid_record, scale_0=1000.0)
    case("badscale_overflow", records=[bad_scale])
    zero_quat = dict(valid_record, rot_0=0.0, rot_1=0.0, rot_2=0.0, rot_3=0.0)
    case("zeroquat", records=[zero_quat])
    cases["truncated_payload"] = ply_bytes([valid_record], degree=1)[:-4]
    for name, data in cases.items():
        source = root / f"{name}.ply"
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(data)
        expect_import_failure(h, name, source)


def test_dropped_records_are_validated(h):
    root = h.output / "cpu" / "invalid"
    cases = {}
    records = [make_record(i) for i in range(7)]
    records[0]["x"] = float("nan")
    cases["dropped_nan_position"] = records
    records = [make_record(i) for i in range(7)]
    records[6]["f_rest_44"] = float("inf")
    cases["dropped_infinite_rest"] = records
    records = [make_record(i) for i in range(7)]
    records[0]["scale_0"] = 1000.0
    cases["dropped_badscale"] = records
    records = [make_record(i) for i in range(7)]
    records[6].update(rot_0=0.0, rot_1=0.0, rot_2=0.0, rot_3=0.0)
    cases["dropped_zeroquat"] = records
    for name, records in cases.items():
        source = root / f"{name}.ply"
        write_ply(source, records)
        expect_import_failure(h, name, source, "--max-count", "1")


def color_record(position, rgb, degree=0):
    record = {name: 0.0 for name in properties_for_degree(degree)}
    record.update(x=position[0], y=position[1], z=position[2], opacity=0.0,
                  scale_0=math.log(0.25), scale_1=math.log(0.25), scale_2=math.log(0.25),
                  rot_0=1.0, rot_1=0.0, rot_2=0.0, rot_3=0.0)
    for axis in range(3):
        record[f"f_dc_{axis}"] = (rgb[axis] - 0.5) / C0
    return record


def make_native_malformed_fixtures(valid_path, root):
    valid = read_hgs(valid_path)
    raw = valid['raw']
    mutations = {}
    def mutate(name, offset, fmt, value):
        changed = bytearray(raw)
        struct.pack_into(fmt, changed, offset, value)
        table_end = 128+32*valid['block_count']
        if 128 <= offset < table_end:
            struct.pack_into('<I',changed,108,codec.crc(changed[128:table_end]))
        struct.pack_into('<I',changed,104,codec.header_crc(changed))
        mutations[name] = bytes(changed)
    mutate('magic', 0, '<4s', b'BAD!')
    mutate('legacy', 0, '<4s', b'HGS1')
    mutate('length',16,'<Q',len(raw)-1)
    mutate('count',32,'<I',2)
    mutate('sh_offsets',64,'<Q',128)
    mutate('storage',40,'<I',99)
    mutate('block_size',44,'<I',0xffffffff)
    mutate('raw_length',140,'<I',0x7fffffff)
    mutate('overlap',128,'<Q',0)
    mutate('reserved',152,'<Q',1)
    mutate('raw_crc',148,'<I',0)
    for name, offset in [('header_crc',104),('table_crc',108)]:
        changed=bytearray(raw);changed[offset]^=1;mutations[name]=changed
    mutations['trailing']=raw+b'x'
    mutations['truncated']=raw[:-1]
    blocks=[bytearray(b['data']) for b in valid['blocks']]
    changed=[bytearray(b) for b in blocks]
    struct.pack_into('<f',changed[0],0,float('nan'))
    mutations['nan']=codec.repack(raw,changed)
    changed=[bytearray(b) for b in blocks]
    if valid['storage']==0:
        changed[0][32:48]=bytes(16)
    else:
        n=valid['blocks'][0]['count']
        rotation=72+12*valid['non_dc_coefficient_count']+10*n
        struct.pack_into('<I',changed[0],rotation,0x1fffffff)
    mutations['zero_quat']=codec.repack(raw,changed)
    def seal(changed):
        end=128+32*valid['block_count']
        struct.pack_into('<I',changed,108,codec.crc(changed[128:end]))
        struct.pack_into('<I',changed,104,codec.header_crc(changed))
        return bytes(changed)
    # GPU fixtures have one block; put junk inside that block's declared compressed span.
    changed=bytearray(raw+b'x')
    struct.pack_into('<Q',changed,16,len(changed))
    struct.pack_into('<I',changed,136,struct.unpack_from('<I',raw,136)[0]+1)
    mutations['compressed_trailing']=seal(changed)
    changed=bytearray(codec.repack(raw,[bytes(blocks[0])+b'x']))
    struct.pack_into('<I',changed,140,len(blocks[0]))
    struct.pack_into('<Q',changed,96,len(blocks[0]))
    mutations['inflate_overrun']=seal(changed)
    changed=[bytearray(b) for b in blocks]
    n=valid['blocks'][0]['count']
    if valid['storage']==1:
        sh=72+12*valid['non_dc_coefficient_count']+17*n
        changed[0][sh]=128  # forbidden signed -128
    else:
        struct.pack_into('<f',changed[0],n*64,float('inf'))
    mutations['sh_code']=codec.repack(raw,changed)
    for name, data in mutations.items():
        (root/f'malformed_{name}.hgs').write_bytes(data)


def test_quantized_hgs(h):
    root=h.output/'quantized'
    root.mkdir(exist_ok=True)
    for count, degree in [(1,0),(255,1),(256,2),(257,3),(513,3)]:
        records=[]
        for i in range(count):
            r=make_record(i%11,degree)
            r.update(x=math.sin(i*.13),y=2.0 if i//256==1 else math.cos(i*.17),z=i*.0123,
                     opacity=(-40 if i%23==0 else 40 if i%19==0 else i%11-5),
                     f_dc_0=(i%17-8)*3.0,f_dc_1=-12.0,f_dc_2=1.2345)
            for j in range(4): r[f'rot_{j}']=(1 if j==i%4 else .13*(j+1))*(-1 if i%2 else 1)
            records.append(r)
        source=root/f'{count}-{degree}.ply'
        write_ply(source,records,degree=degree)
        reference, compressed=root/'reference.hgs',root/'quantized.hgs'
        h.convert(f'hgs_float_{count}',source,reference,'--storage','float32')
        log=h.convert(f'hgs_quantized_{count}',source,compressed,'--storage','quantized')
        a,b=read_hgs(reference),read_hgs(compressed)
        require(a['storage']==0 and b['storage']==1,'HGS2 storage selector')
        require(b['block_count']==(count+255)//256,'HGS2 final partial block')
        require('Quantization clipping: none' in log,'missing adaptive-range diagnostic')
        non_dc=(degree+1)**2-1
        require(b['raw_size']==b['block_count']*(72+12*non_dc)+count*(17+3*non_dc),'HGS2 quantized byte budget')
        require_sequence(a['bounds_min'],b['bounds_min'],'quantized bounds min')
        require_sequence(a['bounds_max'],b['bounds_max'],'quantized bounds max')
        for first in range(0,count,256):
            original=a['base'][first:first+256]
            for local,x in enumerate(original):
                y=b['base'][first+local]
                for j in range(3):
                    low,high=min(v[j] for v in original),max(v[j] for v in original)
                    require(abs(x[j]-y[j]) <= (high-low)/131070+2e-6,'position quantization bound')
                    logs=[math.log(v[4+j]) for v in original]
                    bound=math.expm1((max(logs)-min(logs))/510)+2e-6
                    require(abs(y[4+j]/x[4+j]-1)<=bound,'logarithmic scale error bound')
                    dc=[v[12+j] for v in original]
                    require(abs(x[12+j]-y[12+j]) <= (max(dc)-min(dc))/510+2e-6,'adaptive DC error bound')
                require(abs(x[3]-y[3])<=1/510+2e-6,'linear alpha error bound')
                q=min(math.sqrt(sum((x[j]-y[j])**2 for j in range(8,12))),math.sqrt(sum((x[j]+y[j])**2 for j in range(8,12))))
                require(q<.003,'smallest-three rotation error bound')
                for k in range(non_dc):
                    for channel in range(3):
                        scale=max(abs(a['higher'][i][k][channel]) for i in range(first,min(first+256,count)))
                        require(abs(a['higher'][first+local][k][channel]-b['higher'][first+local][k][channel])<=scale/254+2e-6,'adaptive signed SH error bound')
        again=root/'again.hgs'
        h.convert(f'hgs_deterministic_{count}',source,again,'--storage','quantized')
        require(compressed.read_bytes()==again.read_bytes(),'nondeterministic HGS2 encoding')
    for args in [('--storage','unknown'),('--storage','quantized','--storage','float32'),('--storage',)]:
        compressed.write_bytes(SENTINEL)
        h.convert('hgs_bad_storage_'+str(len(args)),source,compressed,*args,expected='nonzero')
        require(compressed.read_bytes()==SENTINEL,'invalid storage option overwrote output')


def run_gpu_probe(h, storage):
    root = h.output / ("gpu-" + storage)
    root.mkdir(parents=True, exist_ok=True)
    fixtures = {
        "red": ([color_record((0, 0, 3), (1, 0, 0))], 0),
        "green": ([color_record((0, 0, 2), (0, 1, 0))], 0),
        "sh1": ([color_record((0, 0, 3), (0.5, 0, 0), degree=1)], 1),
        "sh3": ([color_record((0, 0, -3), (0, 0, 0), degree=3),
                 color_record((0, 0, 3), (0.5, 0.5, 0), degree=3)], 3),
        "invisible": ([color_record((0, 0, -3), (1, 1, 1)),
                       color_record((100, 0, 3), (1, 1, 1)),
                       color_record((0, 0, 0.01), (1, 1, 1))], 0),
    }
    # Same visible red splat, but 256 culled records force native radix dispatch/padding.
    fixtures["radix_red"] = ([color_record((0, 0, -3), (0, 0, 0)) for _ in range(256)] +
                             [color_record((0, 0, 3), (1, 0, 0))], 0)
    fixtures["sh1"][0][0]["f_rest_2"] = 0.2 / C1  # red, coefficient k=3 (+/- x)
    fixtures["sh3"][0][1]["f_rest_2"] = 0.2 / C1
    fixtures["sh3"][0][1]["f_rest_26"] = 0.2 / (2 * 0.3731763325901154)  # green k=12, channel stride 15
    for name, (records, degree) in fixtures.items():
        source, destination = root / f"{name}.ply", root / f"{name}.hgs"
        write_ply(source, records, degree=degree)
        h.convert(f"gpu_fixture_{storage}_{name}", source, destination, "--sh-degree", str(degree), "--storage", storage)
        header = read_hgs(destination)
        require(header["splat_count"] == len(records) and header["sh_degree"] == degree,
                f"GPU fixture {name}: wrong count or degree")
        require(header["flags"] & 3 == 0, f"GPU fixture {name}: AA/normalization unexpectedly enabled")
    make_native_malformed_fixtures(root / "sh1.hgs", root)
    script = Path(__file__).resolve().with_name("gaussian_splat_native_probe.lua")
    require(script.is_file(), f"missing Lua probe {script}")
    require(h.lua.is_file(), f"Lua executable not found: {h.lua}")
    require(h.shaders.is_dir(), f"compiled shader directory not found: {h.shaders}")
    h.run("gpu_lua_" + storage, [h.lua, script, h.module, h.shaders, root, h.renderer, h.mesh_shaders],
          marker="PASS Gaussian splat native probe:", timeout=180)


def test_gpu_probe(h):
    for storage in ("float32", "quantized"):
        run_gpu_probe(h, storage)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gsplatc", type=Path, required=True, help="native gsplatc executable")
    parser.add_argument("--output", type=Path, required=True, help="persistent fixture/log/report directory")
    parser.add_argument("--lua", type=Path, help="Lua executable for the optional GPU group")
    parser.add_argument("--module", type=Path, help="directory containing the hg_lua harfang module")
    parser.add_argument("--shaders", type=Path, help="compiled native Gaussian shader directory")
    parser.add_argument("--mesh-shaders", type=Path, help="compiled directory containing pos_rgb")
    parser.add_argument("--renderer", choices=("dx11", "metal"), default="metal")
    args = parser.parse_args()
    gpu_args = (args.lua, args.module, args.shaders)
    if any(value is not None for value in gpu_args) and not all(value is not None for value in gpu_args):
        parser.error("--lua, --module and --shaders must be supplied together")
    h = Harness(args)
    tests = [
        ("full-source defaults, explicit reduction and hard budgets", test_full_count_default),
        ("HGS2 quantized blocks, error bounds and determinism", test_quantized_hgs),
        ("SPZ v2/v3 attributes, SH, reduction and gzip options", test_spz_values),
        ("SPZ strict container and payload rejection", test_spz_rejections),
        ("binary .splat decoding, normalization and invalid records", test_binary_splat),
        ("midpoint selection, base values and bounds", test_midpoint_selection_and_values),
        ("SH degrees, channel mapping and missing bands", test_sh_degrees_and_missing_bands),
        ("explicit uniform normalization", test_explicit_normalization),
        ("reordered required PLY properties", test_reordered_required_properties),
        ("invalid PLY schema and atomic failures", test_invalid_ply_schema_and_payload),
        ("validation of dropped source records", test_dropped_records_are_validated),
    ]
    if h.gpu_enabled:
        tests.append(("native Lua loader and GPU pixel probe", test_gpu_probe))
    failures = []
    for name, test in tests:
        try:
            test(h)
        except (TestFailure, OSError, struct.error, subprocess.SubprocessError) as error:
            failures.append({"test": name, "error": str(error)})
            print(f"FAIL {name}: {error}", file=sys.stderr, flush=True)
        else:
            print(f"PASS {name}", flush=True)
    summary = {"status": "failed" if failures else "passed", "tested": len(tests),
               "failures": failures, "commands": h.commands,
               "gpu": h.renderer if h.gpu_enabled else "not requested"}
    report = h.output / "summary.json"
    report.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2), flush=True)
    print(f"Evidence: {report}", flush=True)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

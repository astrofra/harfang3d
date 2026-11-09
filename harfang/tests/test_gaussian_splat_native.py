#!/usr/bin/env python3
"""Native HGS1 converter and optional Lua/GPU integration probe (stdlib only)."""

import argparse
import json
import math
from pathlib import Path
import struct
import subprocess
import sys


C0 = 0.28209479177387814
C1 = 0.4886025119029199
BASE_PROPERTIES = [
    "x", "y", "z", "f_dc_0", "f_dc_1", "f_dc_2", "opacity",
    "scale_0", "scale_1", "scale_2", "rot_0", "rot_1", "rot_2", "rot_3",
]
HEADER_FORMAT = "<4sHHIIQIIIIQQQQ3f3fIIII16s"
HEADER_NAMES = [
    "magic", "major", "minor", "endian_marker", "header_size", "total_size",
    "flags", "sh_degree", "splat_count", "source_count", "base_offset",
    "base_length", "sh_offset", "sh_length", "bounds_min", "bounds_max",
    "base_stride", "sh_stride", "non_dc_coefficient_count", "color_encoding", "reserved",
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
    """Independent, strict HGS1 decoder; no native helper code is mirrored."""
    raw = path.read_bytes()
    require(len(raw) >= 128, f"{path.name}: truncated HGS header")
    values = struct.unpack_from(HEADER_FORMAT, raw, 0)
    grouped = (*values[:14], values[14:17], values[17:20], *values[20:])
    header = dict(zip(HEADER_NAMES, grouped))
    require(struct.calcsize(HEADER_FORMAT) == 128, "internal HGS header format error")
    require(header["magic"] == b"HGS1", f"{path.name}: bad magic")
    require((header["major"], header["minor"]) == (1, 0), f"{path.name}: bad version")
    require(header["endian_marker"] == 0x01020304, f"{path.name}: bad endian marker")
    require(header["header_size"] == 128, f"{path.name}: bad header size")
    require(header["base_offset"] == 128, f"{path.name}: bad base offset")
    require(header["base_stride"] == 64 and header["sh_stride"] == 16,
            f"{path.name}: bad strides")
    require(header["color_encoding"] == 0 and header["reserved"] == bytes(16),
            f"{path.name}: bad color encoding or reserved header bytes")
    require(header["flags"] & ~7 == 0, f"{path.name}: unknown flags")
    degree = header["sh_degree"]
    count = header["splat_count"]
    require(degree <= 3, f"{path.name}: unsupported degree {degree}")
    require(0 < count <= header["source_count"], f"{path.name}: invalid counts")
    non_dc = (degree + 1) ** 2 - 1
    require(header["non_dc_coefficient_count"] == non_dc, f"{path.name}: bad coefficient count")
    require(bool(header["flags"] & 4) == bool(non_dc), f"{path.name}: inconsistent SH flag")
    require(header["base_length"] == count * 64, f"{path.name}: bad base length")
    require(header["sh_offset"] == 128 + count * 64, f"{path.name}: bad SH offset")
    require(header["sh_length"] == count * non_dc * 16, f"{path.name}: bad SH length")
    require(header["total_size"] == 128 + count * 64 + count * non_dc * 16,
            f"{path.name}: bad total size")
    require(len(raw) == header["total_size"], f"{path.name}: file size does not match header")
    base = [struct.unpack_from("<16f", raw, 128 + i * 64) for i in range(count)]
    higher = []
    for splat in range(count):
        records = []
        for coefficient in range(non_dc):
            offset = header["sh_offset"] + (splat * non_dc + coefficient) * 16
            records.append(struct.unpack_from("<4f", raw, offset))
        higher.append(records)
    for splat, record in enumerate(base):
        require(all(math.isfinite(value) for value in record), f"{path.name}: non-finite base record {splat}")
        require(0.0 <= record[3] <= 1.0, f"{path.name}: invalid opacity {record[3]}")
        require(all(value > 0.0 for value in record[4:7]), f"{path.name}: invalid scale")
        require(record[7] == 0.0 and record[15] == 0.0, f"{path.name}: nonzero base padding")
        require_float(math.sqrt(sum(value * value for value in record[8:12])), 1.0,
                      f"{path.name}: quaternion norm")
    for records in higher:
        for record in records:
            require(all(math.isfinite(value) for value in record) and record[3] == 0.0,
                    f"{path.name}: invalid higher SH record")
    header.update(raw=raw, base=base, higher=higher,
                  base_bytes=raw[128:header["sh_offset"]],
                  higher_bytes=raw[header["sh_offset"]:])
    return header


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
        return self.run(label, [self.gsplatc, source, destination, *options], expected=expected)


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
    unknown = root / 'unknown.spz'
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
    valid = valid_path.read_bytes()
    mutations = {}

    def mutate(name, offset, fmt, value):
        data = bytearray(valid)
        struct.pack_into(fmt, data, offset, value)
        mutations[name] = bytes(data)

    data = bytearray(valid)
    data[0:4] = b"BAD!"
    mutations["magic"] = bytes(data)
    mutate("length", 16, "<Q", len(valid) - 1)
    mutate("count", 32, "<I", 2)
    mutate("sh_offsets", 56, "<Q", 128)
    mutate("nan", 128, "<f", float("nan"))
    data = bytearray(valid)
    data[160:176] = bytes(16)
    mutations["zero_quat"] = bytes(data)
    for name, data in mutations.items():
        (root / f"malformed_{name}.hgs").write_bytes(data)


def test_gpu_probe(h):
    root = h.output / "gpu"
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
    fixtures["sh1"][0][0]["f_rest_2"] = 0.2 / C1  # red, coefficient k=3 (+/- x)
    fixtures["sh3"][0][1]["f_rest_2"] = 0.2 / C1
    fixtures["sh3"][0][1]["f_rest_26"] = 0.2 / (2 * 0.3731763325901154)  # green k=12, channel stride 15
    for name, (records, degree) in fixtures.items():
        source, destination = root / f"{name}.ply", root / f"{name}.hgs"
        write_ply(source, records, degree=degree)
        h.convert(f"gpu_fixture_{name}", source, destination, "--sh-degree", str(degree))
        header = read_hgs(destination)
        require(header["splat_count"] == len(records) and header["sh_degree"] == degree,
                f"GPU fixture {name}: wrong count or degree")
        require(header["flags"] & 3 == 0, f"GPU fixture {name}: AA/normalization unexpectedly enabled")
    make_native_malformed_fixtures(root / "sh1.hgs", root)
    script = Path(__file__).resolve().with_name("gaussian_splat_native_probe.lua")
    require(script.is_file(), f"missing Lua probe {script}")
    require(h.lua.is_file(), f"Lua executable not found: {h.lua}")
    require(h.shaders.is_dir(), f"compiled shader directory not found: {h.shaders}")
    h.run("gpu_lua", [h.lua, script, h.module, h.shaders, root, h.renderer, h.mesh_shaders],
          marker="PASS Gaussian splat native probe:", timeout=180)


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

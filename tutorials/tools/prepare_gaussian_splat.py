#!/usr/bin/env python3
"""Offline ``.splat`` to HGS0 Gaussian splat content converter.

Usage::

    python prepare_gaussian_splat.py --source PATH --out DIR [--expected-sha256 HEX]

The converter reads a 32-byte-record ``.splat`` file once, validates every
record, applies one deterministic transform derived from the FULL source
bounds, and writes:

* ``full.bin`` -- every source record in original order.
* ``60k.bin`` / ``30k.bin`` -- deterministic spatial subsets (or the full set
  when the source is smaller than the preset limit).
* ``manifest.json`` -- checksums and conversion metadata.
* ``metadata.lua`` -- the runtime-facing values as a plain ``return { ... }``
  Lua table.

Binary layout (little endian): a 64-byte header followed immediately by
64-byte records, with no padding and no other offsets::

    header: '<4sIII6f6I'  magic, version, count, stride, minXYZ, maxXYZ, 6 reserved
    record: '<16f'        position.xyz/opacity, scale.xyz/0, quaternion WXYZ, RGB/0

Preset selection is a documented spatial approximation only: records are
ordered by a 30-bit Morton key built from 10-bit source-space quantization and
then sampled at midpoint quantiles. It preserves broad spatial coverage but is
not claimed to be an optimal or visually lossless subset.

The module uses only the Python standard library and performs no network or
external tool access. Output is deterministic: no timestamps, absolute paths,
or source attribution data are embedded here.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from pathlib import Path
from typing import Iterable, List, Optional, Sequence, Tuple

MAGIC = b"HGS0"
VERSION = 1
STRIDE = 64
HEADER_STRUCT = struct.Struct("<4sIII6f6I")
RECORD_STRUCT = struct.Struct("<6f8B")
PAYLOAD_STRUCT = struct.Struct("<16f")
HEADER_BYTES = HEADER_STRUCT.size
PAYLOAD_BYTES = PAYLOAD_STRUCT.size
MAX_SOURCE_BYTES = 100_000_000
MAX_RECORDS = 2_000_000
MIN_BOUNDS_HEIGHT = 1e-8
SELECTION = "morton10-midpoint-quantiles-v1"
PRESETS = (("60k", 60000), ("30k", 30000))
MIN_PRESET_LIMIT = min(limit for _, limit in PRESETS)

assert HEADER_BYTES == 64
assert PAYLOAD_BYTES == 64
assert RECORD_STRUCT.size == 32

Vector = Tuple[float, float, float]
Bounds = Tuple[Vector, Vector]

_MORTON_MASK = 0x3FF


class ConversionError(ValueError):
    """Raised when a source or requested output violates the data contract."""


def _f32(value: float, label: str) -> float:
    """Return ``value`` rounded to float32, rejecting non-finite/overflow."""
    if not math.isfinite(value):
        raise ConversionError(f"{label} is not finite")
    try:
        raw = struct.pack("<f", value)
    except (OverflowError, struct.error):
        raise ConversionError(f"{label} is not representable as float32") from None
    return struct.unpack("<f", raw)[0]


def _decode_quaternion(raw: Sequence[int], index: Optional[int] = None) -> Tuple[float, float, float, float]:
    """Decode and normalize the packed WXYZ quaternion bytes.

    Each byte ``b`` maps to ``(b - 128) / 128``. Zero-length and non-finite
    quaternions are rejected before any output is produced.
    """
    w = (raw[0] - 128) / 128.0
    x = (raw[1] - 128) / 128.0
    y = (raw[2] - 128) / 128.0
    z = (raw[3] - 128) / 128.0
    norm_sq = w * w + x * x + y * y + z * z
    label = f"record {index}: " if index is not None else ""
    if not math.isfinite(norm_sq) or norm_sq <= 0.0:
        raise ConversionError(f"{label}zero or non-finite quaternion")
    inverse = 1.0 / math.sqrt(norm_sq)
    normalized = (w * inverse, x * inverse, y * inverse, z * inverse)
    if not all(math.isfinite(component) for component in normalized):
        raise ConversionError(f"{label}zero or non-finite quaternion")
    return normalized


def _source_bounds(data: bytes) -> Bounds:
    """Validate every record and compute the FULL source position bounds."""
    lo = [math.inf, math.inf, math.inf]
    hi = [-math.inf, -math.inf, -math.inf]
    for index, record in enumerate(RECORD_STRUCT.iter_unpack(data)):
        for axis in range(3):
            position = record[axis]
            if not math.isfinite(position):
                raise ConversionError(f"record {index}: non-finite position (axis {axis})")
            if position < lo[axis]:
                lo[axis] = position
            if position > hi[axis]:
                hi[axis] = position
        for axis in range(3):
            scale = record[3 + axis]
            if not math.isfinite(scale):
                raise ConversionError(f"record {index}: non-finite scale (axis {axis})")
            if scale <= 0.0:
                raise ConversionError(f"record {index}: non-positive scale (axis {axis})")
        _decode_quaternion(record[10:14], index)
    return (lo[0], lo[1], lo[2]), (hi[0], hi[1], hi[2])


def _transform_bounds(data: bytes, center: Vector, factor: float) -> Bounds:
    """Validate transformed float32 outputs and return transformed bounds."""
    cx, cy, cz = center
    lo = [math.inf, math.inf, math.inf]
    hi = [-math.inf, -math.inf, -math.inf]
    for index, record in enumerate(RECORD_STRUCT.iter_unpack(data)):
        x, y, z = record[0], record[1], record[2]
        sx, sy, sz = record[3], record[4], record[5]
        transformed = (
            _f32((x - cx) * factor, f"record {index}: transformed position x"),
            _f32(-(y - cy) * factor, f"record {index}: transformed position y"),
            _f32(-(z - cz) * factor, f"record {index}: transformed position z"),
        )
        _f32(sx * factor, f"record {index}: transformed scale x")
        _f32(sy * factor, f"record {index}: transformed scale y")
        _f32(sz * factor, f"record {index}: transformed scale z")
        for axis in range(3):
            if transformed[axis] < lo[axis]:
                lo[axis] = transformed[axis]
            if transformed[axis] > hi[axis]:
                hi[axis] = transformed[axis]
    return (
        (
            _f32(lo[0], "bounds min x"),
            _f32(lo[1], "bounds min y"),
            _f32(lo[2], "bounds min z"),
        ),
        (
            _f32(hi[0], "bounds max x"),
            _f32(hi[1], "bounds max y"),
            _f32(hi[2], "bounds max z"),
        ),
    )


def _encode_record(
    record: Sequence[float],
    center: Vector,
    factor: float,
    index: Optional[int] = None,
) -> bytes:
    """Transform one source record into its 64-byte HGS0 payload."""
    cx, cy, cz = center
    x, y, z = record[0], record[1], record[2]
    sx, sy, sz = record[3], record[4], record[5]
    red, green, blue, alpha = record[6], record[7], record[8], record[9]
    qw, qx, qy, qz = _decode_quaternion(record[10:14], index)
    label = f"record {index}: " if index is not None else ""
    values = (
        _f32((x - cx) * factor, f"{label}transformed position x"),
        _f32(-(y - cy) * factor, f"{label}transformed position y"),
        _f32(-(z - cz) * factor, f"{label}transformed position z"),
        _f32(alpha / 255.0, f"{label}opacity"),
        _f32(sx * factor, f"{label}transformed scale x"),
        _f32(sy * factor, f"{label}transformed scale y"),
        _f32(sz * factor, f"{label}transformed scale z"),
        0.0,
        # Left multiplication by the 180 degree X rotation (w=0, x=1, y=0, z=0).
        _f32(-qx, f"{label}quaternion x"),
        _f32(qw, f"{label}quaternion w"),
        _f32(-qz, f"{label}quaternion z"),
        _f32(qy, f"{label}quaternion y"),
        _f32(red / 255.0, f"{label}color red"),
        _f32(green / 255.0, f"{label}color green"),
        _f32(blue / 255.0, f"{label}color blue"),
        0.0,
    )
    return PAYLOAD_STRUCT.pack(*values)


def _pack_records(
    data: bytes,
    center: Vector,
    factor: float,
    indices: Optional[Iterable[int]] = None,
) -> bytes:
    """Pack all records (or the ascending ``indices`` subset) into payload bytes."""
    output = bytearray()
    if indices is None:
        for record in RECORD_STRUCT.iter_unpack(data):
            output += _encode_record(record, center, factor)
    else:
        for index in indices:
            record = RECORD_STRUCT.unpack_from(data, index * RECORD_STRUCT.size)
            output += _encode_record(record, center, factor, index)
    return bytes(output)


def _quantize_axis(value: float, lo: float, hi: float) -> int:
    extent = hi - lo
    if not extent > 0.0:
        return 0
    quantized = int(math.floor(((value - lo) / extent) * 1024.0))
    if quantized < 0:
        return 0
    if quantized > 1023:
        return 1023
    return quantized


def _spread_10(value: int) -> int:
    """Spread the low 10 bits so each occupies every third bit (30-bit key)."""
    value &= _MORTON_MASK
    value = (value ^ (value << 16)) & 0x030000FF
    value = (value ^ (value << 8)) & 0x0300F00F
    value = (value ^ (value << 4)) & 0x030C30C3
    value = (value ^ (value << 2)) & 0x09249249
    return value


def _morton_key(position: Sequence[float], bounds: Bounds) -> int:
    lo, hi = bounds
    qx = _quantize_axis(position[0], lo[0], hi[0])
    qy = _quantize_axis(position[1], lo[1], hi[1])
    qz = _quantize_axis(position[2], lo[2], hi[2])
    return _spread_10(qx) | (_spread_10(qy) << 1) | (_spread_10(qz) << 2)


def _select_from_keys(keys: Sequence[int], target: int) -> List[int]:
    count = len(keys)
    if target <= 0:
        return []
    if target >= count:
        return list(range(count))
    order = sorted(range(count), key=lambda index: (keys[index], index))
    # floor((j + 0.5) * count / target) expressed with integer arithmetic.
    selected = [order[((2 * j + 1) * count) // (2 * target)] for j in range(target)]
    selected.sort()
    return selected


def _bounds_from_positions(positions: Sequence[Sequence[float]]) -> Bounds:
    lo = [math.inf, math.inf, math.inf]
    hi = [-math.inf, -math.inf, -math.inf]
    for position in positions:
        for axis in range(3):
            value = position[axis]
            if value < lo[axis]:
                lo[axis] = value
            if value > hi[axis]:
                hi[axis] = value
    return (lo[0], lo[1], lo[2]), (hi[0], hi[1], hi[2])


def select_indices(
    positions: Sequence[Sequence[float]],
    target: int,
    bounds: Optional[Bounds] = None,
) -> List[int]:
    """Return ascending original indices for a deterministic spatial subset.

    Indices are ordered by ``(30-bit Morton key, original index)`` using 10-bit
    quantization of the source positions against ``bounds`` (the FULL source
    bounds when supplied). ``target`` midpoint quantiles are then sampled at
    ``floor((j + 0.5) * N / target)`` and the selected original indices are
    sorted ascending so callers keep stable, ascending IDs.
    """
    count = len(positions)
    if target <= 0:
        return []
    if target >= count:
        return list(range(count))
    if bounds is None:
        bounds = _bounds_from_positions(positions)
    keys = [_morton_key(position, bounds) for position in positions]
    return _select_from_keys(keys, target)


def _read_source(source: Path) -> bytes:
    if not source.is_file():
        raise ConversionError(f"source is not a file: {source}")
    try:
        size = source.stat().st_size
    except OSError as error:
        raise ConversionError(f"cannot stat source: {error}") from None
    if size <= 0:
        raise ConversionError("source is empty; expected a positive multiple of 32 bytes")
    if size % RECORD_STRUCT.size != 0:
        raise ConversionError(
            f"source size {size} is not a positive multiple of {RECORD_STRUCT.size} bytes"
        )
    if size >= MAX_SOURCE_BYTES:
        raise ConversionError(
            f"source size {size} must be smaller than {MAX_SOURCE_BYTES} bytes"
        )
    if size // RECORD_STRUCT.size > MAX_RECORDS:
        raise ConversionError(f"source has more than {MAX_RECORDS} records")
    try:
        data = source.read_bytes()
    except OSError as error:
        raise ConversionError(f"cannot read source: {error}") from None
    if len(data) != size:
        raise ConversionError("source changed while being read")
    if not data:
        raise ConversionError("source is empty; expected a positive multiple of 32 bytes")
    return data


def _check_expected_sha256(actual: str, expected: Optional[str]) -> None:
    if expected is None:
        return
    value = expected.strip().lower()
    if len(value) != 64 or any(character not in "0123456789abcdef" for character in value):
        raise ConversionError("expected SHA-256 must be 64 hexadecimal characters")
    if value != actual:
        raise ConversionError(f"source SHA-256 mismatch: expected {value}, got {actual}")


def _lua_number(value: float) -> str:
    number = float(value)
    if not math.isfinite(number):
        raise ConversionError("metadata number is not finite")
    return repr(number)


def _lua_list(values: Sequence[float]) -> str:
    return "{" + ", ".join(_lua_number(value) for value in values) + "}"


def _render_metadata(manifest: dict) -> str:
    """Render the viewer-facing facts as a plain Lua ``return { ... }`` table."""
    lines = [
        "return {",
        f"  version = {manifest['version']},",
        f"  source_count = {manifest['source_count']},",
        f"  sh_degree = {manifest['sh_degree']},",
        '  source_up_axis = "-Y",',
        f"  scale_factor = {_lua_number(manifest['scale_factor'])},",
        f"  source_center = {_lua_list(manifest['source_center'])},",
        f"  bounds_min = {_lua_list(manifest['bounds']['min'])},",
        f"  bounds_max = {_lua_list(manifest['bounds']['max'])},",
        "  presets = {",
    ]
    for name, entry in manifest["presets"].items():
        key = name if name.isidentifier() and not name[0].isdigit() else '["%s"]' % name
        lines.append(
            f'    {key} = {{file = "{entry["file"]}", count = {entry["count"]}, '
            f"offset = {HEADER_BYTES}}},"
        )
    lines.extend(
        [
            "  },",
            f'  selection = "{SELECTION}",',
            "}",
            "",
        ]
    )
    return "\n".join(lines)


def convert(
    source: Path,
    out: Path,
    expected_sha256: Optional[str] = None,
) -> dict:
    """Convert ``source`` into ``out`` and return the manifest dictionary.

    All validation, decoding, hashing, preset selection, and payload encoding
    happen before any filesystem write, so invalid input never leaves a partial
    output artifact behind.
    """
    source = Path(source)
    out = Path(out)

    data = _read_source(source)
    digest = hashlib.sha256(data).hexdigest()
    _check_expected_sha256(digest, expected_sha256)
    count = len(data) // RECORD_STRUCT.size

    source_min, source_max = _source_bounds(data)
    height = source_max[1] - source_min[1]
    if not math.isfinite(height) or height <= MIN_BOUNDS_HEIGHT:
        raise ConversionError(
            f"degenerate source bounds: height {height!r} must exceed {MIN_BOUNDS_HEIGHT!r}"
        )

    center = (
        (source_min[0] + source_max[0]) / 2.0,
        (source_min[1] + source_max[1]) / 2.0,
        (source_min[2] + source_max[2]) / 2.0,
    )
    factor = 2.0 / height

    transformed_min, transformed_max = _transform_bounds(data, center, factor)

    keys: Optional[List[int]] = None
    if count > MIN_PRESET_LIMIT:
        keys = [
            _morton_key((record[0], record[1], record[2]), (source_min, source_max))
            for record in RECORD_STRUCT.iter_unpack(data)
        ]

    full_payload = _pack_records(data, center, factor)

    payloads = {"full": full_payload}
    preset_counts = {"full": count}
    for name, limit in PRESETS:
        target = min(limit, count)
        if target >= count:
            payloads[name] = full_payload
            preset_counts[name] = count
        else:
            if keys is None:  # pragma: no cover - guarded by MIN_PRESET_LIMIT
                raise AssertionError("internal error: missing Morton keys")
            indices = _select_from_keys(keys, target)
            payloads[name] = _pack_records(data, center, factor, indices)
            preset_counts[name] = len(indices)

    presets_section = {}
    for name in ("full",) + tuple(preset_name for preset_name, _ in PRESETS):
        payload = payloads[name]
        presets_section[name] = {
            "file": f"{name}.bin",
            "count": preset_counts[name],
            "sha256": hashlib.sha256(HEADER_STRUCT.pack(MAGIC, VERSION, preset_counts[name], STRIDE, *transformed_min, *transformed_max, 0, 0, 0, 0, 0, 0) + payload).hexdigest(),
            "bytes": HEADER_BYTES + len(payload),
        }

    manifest = {
        "version": VERSION,
        "source_sha256": digest,
        "source_bytes": len(data),
        "source_count": count,
        "sh_degree": 0,
        "source_up_axis": "-Y",
        "scale_factor": _f32(factor, "scale factor"),
        "source_center": [
            _f32(center[0], "source center x"),
            _f32(center[1], "source center y"),
            _f32(center[2], "source center z"),
        ],
        "bounds": {
            "min": list(transformed_min),
            "max": list(transformed_max),
        },
        "presets": presets_section,
        "selection": SELECTION,
    }

    out.mkdir(parents=True, exist_ok=True)
    for name in ("full",) + tuple(preset_name for preset_name, _ in PRESETS):
        # Every binary carries the FULL transformed bounds but its own count.
        header = HEADER_STRUCT.pack(
            MAGIC,
            VERSION,
            preset_counts[name],
            STRIDE,
            transformed_min[0],
            transformed_min[1],
            transformed_min[2],
            transformed_max[0],
            transformed_max[1],
            transformed_max[2],
            0,
            0,
            0,
            0,
            0,
            0,
        )
        (out / presets_section[name]["file"]).write_bytes(header + payloads[name])
    (out / "manifest.json").write_bytes(
        (json.dumps(manifest, indent=2, sort_keys=True, ensure_ascii=True) + "\n").encode("utf-8")
    )
    (out / "metadata.lua").write_bytes(_render_metadata(manifest).encode("utf-8"))

    return manifest


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Convert a .splat file into deterministic HGS0 binaries, "
            "manifest.json, and metadata.lua without network access."
        )
    )
    parser.add_argument("--source", required=True, type=Path, help="input .splat path")
    parser.add_argument("--out", required=True, type=Path, help="output directory")
    parser.add_argument(
        "--expected-sha256",
        default=None,
        help="optional expected SHA-256 of the source file",
    )
    arguments = parser.parse_args(argv)

    try:
        convert(arguments.source, arguments.out, arguments.expected_sha256)
    except (ConversionError, OSError) as error:
        print(f"prepare_gaussian_splat: error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

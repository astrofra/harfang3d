#!/usr/bin/env python3
"""Validate the bounded Tau/Bullet envelope for rb_rings_chain captures."""

import argparse
import json
import math
import sys
from pathlib import Path


def load_capture(path):
    metadata = None
    samples = []
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            record = json.loads(line)
            if record["type"] == "metadata":
                metadata = record
            elif record["type"] == "sample":
                samples.append({body["index"]: body for body in record["bodies"]})
            else:
                raise ValueError(f"{path}:{line_number}: unknown record type")
    if metadata is None:
        raise ValueError(f"{path}: metadata record missing")
    return metadata, samples


def position(body):
    return body["world"][9:12]


def distance(a, b):
    return math.sqrt(sum((left - right) ** 2 for left, right in zip(a, b)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="Bullet rb_rings_chain JSONL capture")
    parser.add_argument("candidate", type=Path, help="Tau rb_rings_chain JSONL capture")
    parser.add_argument("--repeat", type=Path, help="optional repeated Tau capture, required to be byte-identical")
    parser.add_argument("--samples", type=int, default=600, help="required fixed sample count")
    parser.add_argument("--vertical-envelope", type=float, default=5.0, help="maximum Tau/Bullet center-height difference in meters")
    parser.add_argument("--max-speed", type=float, default=20.0, help="maximum finite Tau center speed")
    parser.add_argument("--static-tolerance", type=float, default=0.001, help="maximum drift of the first, static ring")
    args = parser.parse_args()

    reference_metadata, reference_samples = load_capture(args.reference)
    candidate_metadata, candidate_samples = load_capture(args.candidate)
    if reference_metadata["test"] != "rb_rings_chain" or candidate_metadata["test"] != "rb_rings_chain":
        parser.error("both captures must belong to rb_rings_chain")
    if len(reference_samples) < args.samples or len(candidate_samples) < args.samples:
        parser.error(f"captures must contain at least {args.samples} samples")

    reference_samples = reference_samples[: args.samples]
    candidate_samples = candidate_samples[: args.samples]
    body_indices = sorted(reference_samples[0])
    if body_indices != sorted(candidate_samples[0]):
        parser.error("body sets differ")

    failures = []
    max_vertical_error = 0.0
    max_vertical_location = None
    max_speed = 0.0
    max_speed_location = None
    for sample_index, (reference, candidate) in enumerate(zip(reference_samples, candidate_samples)):
        if sorted(reference) != body_indices or sorted(candidate) != body_indices:
            parser.error(f"body set changes at sample {sample_index}")
        for body_index in body_indices:
            body = candidate[body_index]
            values = body["world"] + body["linear_velocity"] + body["angular_velocity"]
            if not all(math.isfinite(value) for value in values):
                failures.append(f"body {body_index} contains a non-finite value at sample {sample_index}")
                continue
            vertical_error = abs(position(reference[body_index])[1] - position(body)[1])
            if vertical_error > max_vertical_error:
                max_vertical_error = vertical_error
                max_vertical_location = (body_index, sample_index)
            speed = math.sqrt(sum(value * value for value in body["linear_velocity"]))
            if speed > max_speed:
                max_speed = speed
                max_speed_location = (body_index, sample_index)

    static_index = body_indices[0]
    static_origin = position(candidate_samples[0][static_index])
    static_drift = max(distance(static_origin, position(sample[static_index])) for sample in candidate_samples)
    if max_vertical_error > args.vertical_envelope:
        failures.append(f"vertical envelope {max_vertical_error:.6g} > {args.vertical_envelope:.6g}")
    if max_speed > args.max_speed:
        failures.append(f"maximum speed {max_speed:.6g} > {args.max_speed:.6g}")
    if static_drift > args.static_tolerance:
        failures.append(f"static-ring drift {static_drift:.6g} > {args.static_tolerance:.6g}")
    repeat_identical = args.repeat is None or args.candidate.read_bytes() == args.repeat.read_bytes()
    if not repeat_identical:
        failures.append("repeated Tau capture is not byte-identical")

    print(f"rb_rings_chain: {args.samples} samples, {len(body_indices) - 1} dynamic rings")
    print(f"maximum vertical error: {max_vertical_error:.6g} m at body/sample {max_vertical_location}")
    print(f"maximum Tau speed: {max_speed:.6g} m/s at body/sample {max_speed_location}")
    print(f"static-ring drift: {static_drift:.6g} m")
    if args.repeat is not None:
        print("repeat capture: byte-identical" if repeat_identical else "repeat capture: mismatch")
    if failures:
        print("Validation failed:", file=sys.stderr)
        print("\n".join(failures), file=sys.stderr)
        return 1
    print("Validation passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

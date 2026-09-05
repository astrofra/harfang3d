#!/usr/bin/env python3
"""Compare two HARFANG physics QA JSONL captures."""

import argparse
import json
import math
import sys
from collections import defaultdict
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
                samples.append(record)
            else:
                raise ValueError(f"{path}:{line_number}: unknown record type")
    if metadata is None:
        raise ValueError(f"{path}: metadata record missing")
    return metadata, samples


def distance(a, b):
    return math.sqrt(sum((left - right) ** 2 for left, right in zip(a, b)))


def orientation_error(a, b):
    # World matrices are stored as three orthonormal basis columns followed by translation.
    trace = 0.0
    for column in range(3):
        start = column * 3
        left, right = a[start : start + 3], b[start : start + 3]
        length = math.sqrt(sum(value * value for value in left) * sum(value * value for value in right))
        trace += sum(x * y for x, y in zip(left, right)) / length if length else 1.0
    cosine = max(-1.0, min(1.0, (trace - 1.0) * 0.5))
    return math.degrees(math.acos(cosine))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="Bullet ground-truth JSONL capture")
    parser.add_argument("candidate", type=Path, help="Candidate backend JSONL capture")
    parser.add_argument("--position-threshold", type=float, default=0.05, help="maximum accepted position error in meters")
    parser.add_argument("--rotation-threshold", type=float, default=5.0, help="maximum accepted orientation error in degrees")
    parser.add_argument("--linear-velocity-threshold", type=float, default=0.1, help="maximum accepted linear velocity error")
    parser.add_argument("--angular-velocity-threshold", type=float, default=0.1, help="maximum accepted angular velocity error")
    args = parser.parse_args()

    reference_metadata, reference_samples = load_capture(args.reference)
    candidate_metadata, candidate_samples = load_capture(args.candidate)
    if reference_metadata["test"] != candidate_metadata["test"]:
        parser.error("captures belong to different QA scenarios")

    count = min(len(reference_samples), len(candidate_samples))
    if count == 0:
        parser.error("captures contain no samples")
    if len(reference_samples) != len(candidate_samples):
        print(f"warning: sample count differs ({len(reference_samples)} reference, {len(candidate_samples)} candidate); comparing {count}", file=sys.stderr)

    maxima = defaultdict(float)
    sums = defaultdict(float)
    worst = {}
    for sample_index in range(count):
        reference = {body["index"]: body for body in reference_samples[sample_index]["bodies"]}
        candidate = {body["index"]: body for body in candidate_samples[sample_index]["bodies"]}
        if reference.keys() != candidate.keys():
            parser.error(f"body set differs at sample {sample_index}")
        for index in reference:
            a, b = reference[index], candidate[index]
            values = {
                "position": distance(a["world"][9:12], b["world"][9:12]),
                "rotation": orientation_error(a["world"], b["world"]),
                "linear_velocity": distance(a["linear_velocity"], b["linear_velocity"]),
                "angular_velocity": distance(a["angular_velocity"], b["angular_velocity"]),
            }
            for name, value in values.items():
                key = (index, name)
                sums[key] += value
                if key not in worst or value > maxima[key]:
                    maxima[key] = value
                    worst[key] = sample_index

    print(f"QA scenario: {reference_metadata['test']}")
    print(f"Compared {count} samples: {reference_metadata['backend']} -> {candidate_metadata['backend']}")
    failures = []
    thresholds = {
        "position": args.position_threshold,
        "rotation": args.rotation_threshold,
        "linear_velocity": args.linear_velocity_threshold,
        "angular_velocity": args.angular_velocity_threshold,
    }
    for index in sorted({key[0] for key in maxima}):
        print(f"body {index}:")
        for name in ("position", "rotation", "linear_velocity", "angular_velocity"):
            key = (index, name)
            print(f"  {name}: mean={sums[key] / count:.6g}, max={maxima[key]:.6g} at sample {worst[key]}")
            if maxima[key] > thresholds[name]:
                failures.append(f"body {index} {name} {maxima[key]:.6g} > {thresholds[name]:.6g}")
    if failures:
        print("Comparison exceeds thresholds:", file=sys.stderr)
        print("\n".join(failures), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Measure and plot Bullet/Tau motion amplitude for the impulse-callback QA."""

import argparse
import json
import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


COLORS = {
    "bullet": "#3f8cff",
    "tau": "#ff4b4b",
    "corrected": "#35d07f",
    "target": "#b8b8b8",
}


def load_capture(path, body_index):
    metadata = None
    samples = []
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            record = json.loads(line)
            if record["type"] == "metadata":
                metadata = record
            elif record["type"] == "sample":
                bodies = {body["index"]: body for body in record["bodies"]}
                if body_index not in bodies:
                    raise ValueError(f"{path}:{line_number}: body {body_index} missing")
                body = bodies[body_index]
                samples.append(
                    {
                        "time": record["time"],
                        "y": body["world"][10],
                        "vy": body["linear_velocity"][1],
                    }
                )
            else:
                raise ValueError(f"{path}:{line_number}: unknown record type")
    if metadata is None:
        raise ValueError(f"{path}: metadata record missing")
    return metadata, samples


def amplitude(values):
    low, high = min(values), max(values)
    return low, high, high - low


def errors(reference, candidate):
    residuals = [right - left for left, right in zip(reference, candidate)]
    return {
        "residuals": residuals,
        "mean_abs": sum(abs(value) for value in residuals) / len(residuals),
        "rms": math.sqrt(sum(value * value for value in residuals) / len(residuals)),
        "max_abs": max(abs(value) for value in residuals),
    }


def print_amplitude(label, values):
    low, high, peak_to_peak = amplitude(values)
    print(f"{label}: min={low:.9g} m max={high:.9g} m peak-to-peak={peak_to_peak:.9g} m")
    return peak_to_peak


def style_axis(axis):
    axis.set_facecolor("#090909")
    axis.grid(color="#292929", linewidth=0.6, alpha=0.8)
    axis.tick_params(colors="#c8c8c8")
    for spine in axis.spines.values():
        spine.set_color("#555555")
    axis.xaxis.label.set_color("#dddddd")
    axis.yaxis.label.set_color("#dddddd")
    axis.title.set_color("white")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="Bullet JSONL capture")
    parser.add_argument("candidate", type=Path, help="Tau JSONL capture")
    parser.add_argument("--body", type=int, default=1, help="body index to plot")
    parser.add_argument("--target-before", type=float, default=2.0, help="target Y before the transition")
    parser.add_argument("--target-after", type=float, default=4.358526746242, help="target Y after the transition")
    parser.add_argument("--target-change", type=float, default=5.0, help="target transition time in seconds")
    parser.add_argument(
        "--history-weight",
        type=float,
        default=0.5,
        help="previous-step weight for the proposed display-only trajectory",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("qa_dumps/rb_dynamic_impulse_callback_amplitude.png"),
        help="output PNG path",
    )
    args = parser.parse_args()

    reference_metadata, reference = load_capture(args.reference, args.body)
    candidate_metadata, candidate = load_capture(args.candidate, args.body)
    if reference_metadata["test"] != candidate_metadata["test"]:
        parser.error("captures belong to different QA scenarios")
    if len(reference) != len(candidate):
        parser.error(f"sample count differs ({len(reference)} reference, {len(candidate)} candidate)")
    if not reference:
        parser.error("captures contain no samples")
    if not 0.0 <= args.history_weight <= 1.0:
        parser.error("--history-weight must be between 0 and 1")

    times = [sample["time"] for sample in reference]
    bullet_y = [sample["y"] for sample in reference]
    tau_y = [sample["y"] for sample in candidate]
    bullet_vy = [sample["vy"] for sample in reference]
    tau_vy = [sample["vy"] for sample in candidate]
    target_y = [args.target_before if time < args.target_change else args.target_after for time in times]

    # Model a display-only interpolation between Tau's previous and current
    # fixed-step states. A half-step is deliberately used as a conservative,
    # backend-independent proposal rather than fitting a magic gain to Bullet.
    previous_tau_y = [args.target_before] + tau_y[:-1]
    previous_tau_vy = [0.0] + tau_vy[:-1]
    corrected_tau_y = [
        current * (1.0 - args.history_weight) + previous * args.history_weight
        for current, previous in zip(tau_y, previous_tau_y)
    ]
    corrected_tau_vy = [
        current * (1.0 - args.history_weight) + previous * args.history_weight
        for current, previous in zip(tau_vy, previous_tau_vy)
    ]

    bullet_amplitude = print_amplitude("Bullet full capture", bullet_y)
    tau_amplitude = print_amplitude("Tau full capture", tau_y)
    print(f"Amplitude delta (Tau - Bullet): {tau_amplitude - bullet_amplitude:+.9g} m")

    before = [index for index, time in enumerate(times) if time < args.target_change]
    after = [index for index, time in enumerate(times) if time >= args.target_change]
    print_amplitude("Bullet before target change", [bullet_y[index] for index in before])
    print_amplitude("Tau before target change", [tau_y[index] for index in before])
    print_amplitude("Bullet after target change", [bullet_y[index] for index in after])
    print_amplitude("Tau after target change", [tau_y[index] for index in after])

    raw_error = errors(bullet_y, tau_y)
    corrected_error = errors(bullet_y, corrected_tau_y)
    print(
        "Raw Tau position error: "
        f"mean_abs={raw_error['mean_abs']:.9g} m rms={raw_error['rms']:.9g} m max={raw_error['max_abs']:.9g} m"
    )
    print_amplitude("Proposed display trajectory", corrected_tau_y)
    print(
        f"Tau display interpolation proposal (history weight {args.history_weight:.3f}): "
        f"mean_abs={corrected_error['mean_abs']:.9g} m rms={corrected_error['rms']:.9g} m "
        f"max={corrected_error['max_abs']:.9g} m"
    )
    print(f"Maximum-error reduction: {(1.0 - corrected_error['max_abs'] / raw_error['max_abs']) * 100.0:.3f}%")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    figure, axes = plt.subplots(3, 1, figsize=(14, 10), sharex=True, facecolor="black", gridspec_kw={"height_ratios": [2.3, 1.3, 1.2]})
    for axis in axes:
        style_axis(axis)
        axis.axvline(args.target_change, color="#777777", linewidth=0.8, linestyle=":")

    axes[0].plot(times, target_y, color=COLORS["target"], linewidth=1.0, linestyle=":", label="Target")
    axes[0].plot(times, bullet_y, color=COLORS["bullet"], linewidth=1.5, label=f"Bullet (A={bullet_amplitude:.3f} m)")
    axes[0].plot(times, tau_y, color=COLORS["tau"], linewidth=1.2, label=f"Tau (A={tau_amplitude:.3f} m)")
    axes[0].plot(
        times,
        corrected_tau_y,
        color=COLORS["corrected"],
        linewidth=1.0,
        linestyle="--",
        label=f"Tau proposed display path ({args.history_weight:.1f}-tick delay)",
    )
    axes[0].set_title(f"Motion amplitude: {reference_metadata['test']} — body {args.body}")
    axes[0].set_ylabel("Vertical position Y (m)")
    axes[0].legend(facecolor="#111111", edgecolor="#555555", labelcolor="white", loc="best")

    axes[1].plot(times, bullet_vy, color=COLORS["bullet"], linewidth=1.3, label="Bullet")
    axes[1].plot(times, tau_vy, color=COLORS["tau"], linewidth=1.0, label="Tau")
    axes[1].plot(
        times,
        corrected_tau_vy,
        color=COLORS["corrected"],
        linewidth=0.9,
        linestyle="--",
        label=f"Tau proposed ({args.history_weight:.1f}-tick delay)",
    )
    axes[1].set_ylabel("Vertical velocity (m/s)")
    axes[1].legend(facecolor="#111111", edgecolor="#555555", labelcolor="white", loc="best")

    axes[2].plot(times, [value * 100.0 for value in raw_error["residuals"]], color=COLORS["tau"], linewidth=1.0, label="Tau - Bullet")
    axes[2].plot(
        times,
        [value * 100.0 for value in corrected_error["residuals"]],
        color=COLORS["corrected"],
        linewidth=0.9,
        linestyle="--",
        label="Proposed - Bullet",
    )
    axes[2].axhline(0.0, color="#777777", linewidth=0.7)
    axes[2].set_ylabel("Position residual (cm)")
    axes[2].set_xlabel("Time (s)")
    axes[2].legend(facecolor="#111111", edgecolor="#555555", labelcolor="white", loc="best")

    figure.tight_layout()
    figure.savefig(args.output, dpi=180, facecolor=figure.get_facecolor())
    plt.close(figure)
    print(f"Amplitude plot written to: {args.output}")


if __name__ == "__main__":
    main()

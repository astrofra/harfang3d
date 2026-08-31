#!/usr/bin/env python3
"""Render Bullet and Tau QA trajectories as an overlaid 3D PNG."""

import argparse
import json
from pathlib import Path

import matplotlib.pyplot as plt


BACKEND_COLORS = {"bullet": "#3f8cff", "tau": "#ff4b4b"}


def load_capture(path):
    metadata = None
    trajectories = {}
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            record = json.loads(line)
            if record["type"] == "metadata":
                metadata = record
            elif record["type"] == "sample":
                for body in record["bodies"]:
                    trajectories.setdefault(body["index"], []).append(body["world"][9:12])
            else:
                raise ValueError(f"{path}:{line_number}: unknown record type")
    if metadata is None:
        raise ValueError(f"{path}: metadata record missing")
    return metadata, trajectories


def style_axis(axis):
    axis.set_facecolor("black")
    axis.xaxis.pane.set_facecolor("black")
    axis.yaxis.pane.set_facecolor("black")
    axis.zaxis.pane.set_facecolor("black")
    axis.xaxis.pane.set_edgecolor("#404040")
    axis.yaxis.pane.set_edgecolor("#404040")
    axis.zaxis.pane.set_edgecolor("#404040")
    axis.tick_params(colors="#b0b0b0")
    axis.xaxis.label.set_color("#d0d0d0")
    axis.yaxis.label.set_color("#d0d0d0")
    axis.zaxis.label.set_color("#d0d0d0")
    axis.grid(color="#303030", linewidth=0.5)


def draw_trajectories(axis, trajectories, color, label, linewidth):
    for index, positions in sorted(trajectories.items()):
        x, y, z = zip(*positions)
        # Matplotlib's third coordinate is rendered as vertical. Preserve Harfang's Y-up convention.
        axis.plot(x, z, y, color=color, linewidth=linewidth, alpha=0.9, label=label if index == 1 else None)
        axis.scatter(x[0], z[0], y[0], color=color, s=22, marker="o", depthshade=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="Bullet JSONL capture")
    parser.add_argument("candidate", type=Path, help="Tau JSONL capture")
    parser.add_argument("--output", type=Path, default=Path("qa_dumps/trajectory_comparison.png"), help="output PNG path")
    parser.add_argument("--elevation", type=float, default=22.0, help="3D camera elevation in degrees")
    parser.add_argument("--azimuth", type=float, default=122.0, help="3D camera azimuth in degrees")
    args = parser.parse_args()

    reference_metadata, reference = load_capture(args.reference)
    candidate_metadata, candidate = load_capture(args.candidate)
    if reference_metadata["test"] != candidate_metadata["test"]:
        parser.error("captures belong to different QA scenarios")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    figure = plt.figure(figsize=(12, 9), facecolor="black")
    axis = figure.add_subplot(111, projection="3d")
    style_axis(axis)
    draw_trajectories(axis, reference, BACKEND_COLORS["bullet"], "Bullet", 5.4)
    draw_trajectories(axis, candidate, BACKEND_COLORS["tau"], "Tau", 1.8)

    axis.set_title(f"Physics QA trajectories: {reference_metadata['test']}", color="white", pad=18)
    axis.set_xlabel("X")
    axis.set_ylabel("Z")
    axis.set_zlabel("Y (vertical)")
    axis.view_init(elev=args.elevation, azim=args.azimuth)
    legend = axis.legend(facecolor="#101010", edgecolor="#606060", labelcolor="white", loc="upper left")
    for handle in legend.legend_handles:
        handle.set_alpha(1.0)
    figure.tight_layout()
    figure.savefig(args.output, dpi=180, facecolor=figure.get_facecolor())
    print(f"Trajectory plot written to: {args.output}")


if __name__ == "__main__":
    main()

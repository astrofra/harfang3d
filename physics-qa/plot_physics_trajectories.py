#!/usr/bin/env python3
"""Render Bullet and Tau QA trajectories as an overlaid 3D PNG."""

import argparse
from io import BytesIO
import json
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from PIL import Image, ImageChops


BACKEND_COLORS = {"bullet": "#3f8cff", "tau": "#ff4b4b"}
TRAJECTORY_LINEWIDTH = 0.9
DOT_RATIO = 0.05


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
    axis.xaxis.pane.set_edgecolor("#202020")
    axis.yaxis.pane.set_edgecolor("#202020")
    axis.zaxis.pane.set_edgecolor("#202020")
    axis.tick_params(colors="#b0b0b0")
    axis.xaxis.label.set_color("#d0d0d0")
    axis.yaxis.label.set_color("#d0d0d0")
    axis.zaxis.label.set_color("#d0d0d0")
    axis.grid(color="#181818", linewidth=0.5)


def get_limits(*trajectory_sets):
    coordinates = [position for trajectories in trajectory_sets for positions in trajectories.values() for position in positions]
    x, y, z = zip(*coordinates)
    values = (x, z, y)  # Matplotlib's third coordinate is vertical; Harfang's Y stays up.
    limits = []
    for axis_values in values:
        low, high = min(axis_values), max(axis_values)
        margin = max((high - low) * 0.05, 0.1)
        limits.append((low - margin, high + margin))
    return limits


def configure_axis(axis, limits, elevation, azimuth):
    axis.set_xlim(limits[0])
    axis.set_ylim(limits[1])
    axis.set_zlim(limits[2])
    axis.view_init(elev=elevation, azim=azimuth)


def draw_trajectories(axis, trajectories, color, linewidth):
    for index, positions in sorted(trajectories.items()):
        x, y, z = zip(*positions)
        axis.plot(x, z, y, color=color, linewidth=linewidth, alpha=1.0)
        dot_count = max(2, round(len(positions) * DOT_RATIO))
        dot_indices = {round(index * (len(positions) - 1) / (dot_count - 1)) for index in range(dot_count)}
        axis.scatter([x[index] for index in dot_indices], [z[index] for index in dot_indices], [y[index] for index in dot_indices],
                     color=color, s=6, marker="o", depthshade=False)


def render_layer(trajectories, color, linewidth, limits, elevation, azimuth):
    figure = plt.figure(figsize=(12, 9), facecolor=(0, 0, 0, 0))
    axis = figure.add_axes((0.08, 0.08, 0.84, 0.84), projection="3d")
    axis.set_axis_off()
    axis.patch.set_alpha(0.0)
    configure_axis(axis, limits, elevation, azimuth)
    draw_trajectories(axis, trajectories, color, linewidth)
    output = BytesIO()
    figure.savefig(output, format="png", dpi=180, transparent=True)
    plt.close(figure)
    layer = Image.open(output).convert("RGBA")
    black = Image.new("RGB", layer.size, "black")
    black.paste(layer, mask=layer.getchannel("A"))
    return black


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
    limits = get_limits(reference, candidate)
    figure = plt.figure(figsize=(12, 9), facecolor="black")
    axis = figure.add_axes((0.08, 0.08, 0.84, 0.84), projection="3d")
    style_axis(axis)
    configure_axis(axis, limits, args.elevation, args.azimuth)

    axis.set_title(f"Physics QA trajectories: {reference_metadata['test']}", color="white", pad=18)
    axis.set_xlabel("X")
    axis.set_ylabel("Z")
    axis.set_zlabel("Y (vertical)")
    axis.legend(handles=[Line2D([0], [0], color=BACKEND_COLORS["bullet"], linewidth=TRAJECTORY_LINEWIDTH, label="Bullet"),
                         Line2D([0], [0], color=BACKEND_COLORS["tau"], linewidth=TRAJECTORY_LINEWIDTH, label="Tau")],
                facecolor="#101010", edgecolor="#606060", labelcolor="white", loc="upper left")

    base = BytesIO()
    figure.savefig(base, format="png", dpi=180, facecolor=figure.get_facecolor())
    plt.close(figure)
    bullet_layer = render_layer(reference, BACKEND_COLORS["bullet"], TRAJECTORY_LINEWIDTH, limits, args.elevation, args.azimuth)
    tau_layer = render_layer(candidate, BACKEND_COLORS["tau"], TRAJECTORY_LINEWIDTH, limits, args.elevation, args.azimuth)
    image = ImageChops.add(Image.open(base).convert("RGB"), bullet_layer)
    image = ImageChops.add(image, tau_layer)
    image.save(args.output)
    print(f"Trajectory plot written to: {args.output}")


if __name__ == "__main__":
    main()

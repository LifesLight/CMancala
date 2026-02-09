#!/usr/bin/env python3
import csv
import sys
import os
import random
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

def read_depth_time(path):
    depth = []
    time = []

    with open(path, newline='') as f:
        reader = csv.DictReader(f)

        if "Depth" not in reader.fieldnames or "Time" not in reader.fieldnames:
            raise ValueError(f"{path}: expected columns 'Depth,Time'")

        for row in reader:
            try:
                d = int(row["Depth"])
                t = float(row["Time"])
            except Exception:
                continue

            if t > 0.0:
                depth.append(d)
                time.append(t)

    return depth, time

def cumulative(values):
    out = []
    total = 0.0
    for v in values:
        total += v
        out.append(total)
    return out

def main(files):
    if not files:
        print("Usage: VisualizeBenchmark.py file1.csv [file2.csv ...]")
        sys.exit(1)

    fig, ax = plt.subplots(figsize=(16, 10))
    ax.set_yscale("log")
    ax.set_xlabel("Depth")
    ax.set_ylabel("Time (s, log)")
    ax.set_title("Benchmark Results")

    all_depths = []
    all_times = []

    cmap = plt.get_cmap("tab20")
    colors = list(cmap.colors)
    random.shuffle(colors)

    plotted = False

    for i, path in enumerate(files):
        try:
            x, y = read_depth_time(path)
        except Exception as e:
            print(e)
            continue

        if not x:
            continue

        label = os.path.basename(path)
        color = colors[i % len(colors)]

        # cumulative (background)
        y_acc = cumulative(y)
        ax.semilogy(
            x, y_acc,
            linestyle=":",
            linewidth=2.5,
            color=color,
            alpha=0.8,
            zorder=1
        )

        # per-depth (foreground)
        ax.semilogy(
            x, y,
            marker="o",
            linestyle="-",
            linewidth=2,
            color=color,
            label=label,
            zorder=2
        )

        all_depths.extend(x)
        all_times.extend(y)
        all_times.extend(y_acc)
        plotted = True

    if not plotted:
        print("No valid data to plot.")
        sys.exit(1)

    ax.set_xlim(0, max(all_depths))
    ax.set_ylim(min(all_times) * 0.9, max(all_times) * 1.1)
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))

    ax.grid(True, which="both", ls=":")
    ax.legend(
        loc="center left",
        bbox_to_anchor=(1.02, 0.5),
        fontsize="small",
        frameon=False
    )

    plt.tight_layout(rect=(0, 0, 0.85, 1))
    plt.show()

if __name__ == "__main__":
    main(sys.argv[1:])

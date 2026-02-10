#!/usr/bin/env python3
import csv
import sys
import os
import random
import argparse
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

def read_depth_time(path):
    depth = []
    time = []

    with open(path, newline="") as f:
        reader = csv.DictReader(f)

        if "Depth" not in reader.fieldnames or "Time" not in reader.fieldnames:
            raise ValueError(f"{path}: expected columns 'Depth,Time'")

        for row in reader:
            try:
                d = int(row["Depth"])
                t = float(row["Time"])
            except Exception:
                continue

            # Log scales don't play well with 0 or negative numbers,
            # so we keep the filter strict.
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

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("files", nargs="+")
    parser.add_argument(
        "--log-factor",
        type=float,
        default=10.0,
        help="Log base for Y-axis scaling. Set to 0 for linear scale. (Default: 10)"
    )
    parser.add_argument(
        "--cumulative",
        action="store_true",
        help="Enable plotting of the cumulative time curve (Default: Off)"
    )
    args = parser.parse_args()

    fig, ax = plt.subplots(figsize=(16, 10))

    # Configure Y-Axis Scale
    if args.log_factor > 0:
        ax.set_yscale("log", base=args.log_factor)
        # Clean label formatting based on if it's an integer-like float or not
        if args.log_factor.is_integer():
            base_label = int(args.log_factor)
        else:
            base_label = args.log_factor
        ax.set_ylabel(f"Time (s, log base {base_label})")
    else:
        ax.set_yscale("linear")
        ax.set_ylabel("Time (s)")

    ax.set_xlabel("Depth")
    ax.set_title("Time Per Depth Step")

    all_depths = []
    all_times = []

    cmap = plt.get_cmap("tab20")
    colors = list(cmap.colors)
    random.shuffle(colors)

    plotted = False

    for i, path in enumerate(args.files):
        try:
            x, y = read_depth_time(path)
        except Exception as e:
            print(f"Error reading {path}: {e}")
            continue

        if not x:
            continue

        #label = os.path.basename(path)
        label = path
        color = colors[i % len(colors)]

        # 1. Plot Cumulative (Background) - Only if flag is set
        if args.cumulative:
            y_acc = cumulative(y)
            ax.plot(
                x, y_acc,
                linestyle=":",
                linewidth=2.5,
                color=color,
                alpha=0.8,
                zorder=1
            )
            all_times.extend(y_acc)

        # 2. Plot Per-Depth (Foreground)
        if len(x) == 1:
            # Single-point run → horizontal line
            y0 = y[0]
            xmin = 0
            xmax = max(1, max(all_depths, default=1))

            ax.hlines(
                y0,
                xmin,
                xmax,
                linestyle="-",
                linewidth=2,
                color=color,
                label=label,
                zorder=2
            )

            ax.plot(
                x, y,
                marker="o",
                color=color,
                zorder=3
            )
        else:
            ax.plot(
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
        plotted = True

    if not plotted:
        print("No valid data to plot.")
        sys.exit(1)

    # Set Limits
    ax.set_xlim(0, max(all_depths))
    
    # Calculate Y-limits dynamically
    y_min = min(all_times)
    y_max = max(all_times)
    
    # Add a little padding to the view
    if args.log_factor > 0:
        ax.set_ylim(y_min * 0.9, y_max * 1.1)
    else:
        # Linear scale padding
        y_range = y_max - y_min
        ax.set_ylim(max(0, y_min - (y_range * 0.05)), y_max + (y_range * 0.05))

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
    main()

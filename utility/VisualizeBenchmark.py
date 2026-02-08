#!/usr/bin/env python3
import csv
import sys
import os
import matplotlib.pyplot as plt
from itertools import cycle

def valid_xy(depths, values):
    pairs = []
    for d, v in zip(depths, values):
        try:
            dv = float(v)
            dd = int(d)
        except Exception:
            continue
        if dv > 0.0:
            pairs.append((dd, dv))
    if not pairs:
        return [], []
    pairs.sort(key=lambda p: p[0])
    x, y = zip(*pairs)
    return list(x), list(y)

def read_cols(path):
    with open(path, newline='') as f:
        reader = csv.DictReader(f)
        # collect columns as strings (preserve original order)
        depth = []
        lu = []
        lc = []
        gu = []
        gc = []
        for row in reader:
            # keep missing fields empty
            depth.append(row.get("Depth", "") or row.get("depth",""))
            lu.append(row.get("Local_Unclipped", "") or row.get("local_unclipped",""))
            lc.append(row.get("Local_Clipped", "") or row.get("local_clipped",""))
            gu.append(row.get("Global_Unclipped", "") or row.get("global_unclipped",""))
            gc.append(row.get("Global_Clipped", "") or row.get("global_clipped",""))
    return depth, lu, lc, gu, gc

def main(files):
    if not files:
        print("Usage: VisualizeBenchmark.py file1.csv [file2.csv ...]")
        sys.exit(1)

    fig, axs = plt.subplots(1, 2, figsize=(12, 5), sharey=True)
    colors = cycle(plt.rcParams['axes.prop_cycle'].by_key()['color'])
    any_plotted = False

    for path, color in zip(files, colors):
        label_base = os.path.basename(path)
        try:
            depth, lu, lc, gu, gc = read_cols(path)
        except FileNotFoundError:
            print(f"File not found: {path}")
            continue

        x_lu, y_lu = valid_xy(depth, lu)
        x_lc, y_lc = valid_xy(depth, lc)
        x_gu, y_gu = valid_xy(depth, gu)
        x_gc, y_gc = valid_xy(depth, gc)

        # LOCAL subplot
        if x_lu:
            axs[0].semilogy(x_lu, y_lu, marker='o', linestyle='-', label=f"{label_base} Local Unclipped", color=color)
            any_plotted = True
        if x_lc:
            axs[0].semilogy(x_lc, y_lc, marker='s', linestyle='--', label=f"{label_base} Local Clipped", color=color)

        # GLOBAL subplot
        if x_gu:
            axs[1].semilogy(x_gu, y_gu, marker='o', linestyle='-', label=f"{label_base} Global Unclipped", color=color)
            any_plotted = True
        if x_gc:
            axs[1].semilogy(x_gc, y_gc, marker='s', linestyle='--', label=f"{label_base} Global Clipped", color=color)

    if not any_plotted:
        print("No positive (non-zero) data found to plot.")
        sys.exit(1)

    axs[0].set_xlabel("Depth")
    axs[0].set_ylabel("Time per depth (s, log)")
    axs[0].set_title("LOCAL Solver Benchmark")
    axs[0].legend(fontsize="small")
    axs[0].grid(True, which="both", ls=":")

    axs[1].set_xlabel("Depth")
    axs[1].set_title("GLOBAL Solver Benchmark")
    axs[1].legend(fontsize="small")
    axs[1].grid(True, which="both", ls=":")

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main(sys.argv[1:])

import csv
import matplotlib.pyplot as plt

def valid_xy(depth, values):
    x = []
    y = []
    for d, v in zip(depth, values):
        if v >= 0.0:
            x.append(d)
            y.append(v)
    return x, y

depth = []
local_u = []
local_c = []
global_u = []
global_c = []

with open("cmancala_benchmark.txt") as f:
    reader = csv.DictReader(f)
    for row in reader:
        depth.append(int(row["Depth"]))
        local_u.append(float(row["Local_Unclipped"]))
        local_c.append(float(row["Local_Clipped"]))
        global_u.append(float(row["Global_Unclipped"]))
        global_c.append(float(row["Global_Clipped"]))

# --- LOCAL ---
x_lu, y_lu = valid_xy(depth, local_u)
x_lc, y_lc = valid_xy(depth, local_c)

plt.figure()
plt.plot(x_lu, y_lu, label="Local Unclipped")
plt.plot(x_lc, y_lc, label="Local Clipped")
plt.yscale("log")
plt.xlabel("Depth")
plt.ylabel("Time per depth (s, log)")
plt.title("LOCAL Solver Benchmark")
plt.legend()
plt.grid(True)

# --- GLOBAL ---
x_gu, y_gu = valid_xy(depth, global_u)
x_gc, y_gc = valid_xy(depth, global_c)

plt.figure()
plt.plot(x_gu, y_gu, label="Global Unclipped")
plt.plot(x_gc, y_gc, label="Global Clipped")
plt.yscale("log")
plt.xlabel("Depth")
plt.ylabel("Time per depth (s, log)")
plt.title("GLOBAL Solver Benchmark")
plt.legend()
plt.grid(True)

plt.show()
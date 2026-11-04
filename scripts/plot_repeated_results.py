import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from utils import save_plot

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"

csv_file = RESULTS / "repeated_benchmark_results.csv"
if not csv_file.exists():
    csv_file = RESULTS / "repeated_benchmark_after_optimization.csv"

if not csv_file.exists():
    print(f"[ERROR] No benchmark CSV found at {csv_file}")
    exit(1)

print(f"Reading benchmark data from: {csv_file}")
data = pd.read_csv(csv_file)

threads = data["threads"]
serial_col = "serial_avg_ms" if "serial_avg_ms" in data.columns else "serial_time_ms"
parallel_col = "parallel_avg_ms" if "parallel_avg_ms" in data.columns else "parallel_time_ms"

serial_time = data[serial_col].iloc[0]
parallel_time = data[parallel_col]
speedup = data["speedup"]
efficiency = data["efficiency"] * (100.0 if data["efficiency"].max() <= 1.0 else 1.0)

# Graph 1: Execution Time
plt.figure(figsize=(8, 5))
plt.plot(threads, parallel_time, marker="o", linewidth=2, label="Parallel Time")
plt.axhline(serial_time, linestyle="--", color="red", linewidth=1.5, label=f"Serial Baseline ({serial_time:.1f} ms)")
plt.xlabel("Number of Threads")
plt.ylabel("Execution Time (ms)")
plt.title("Execution Time vs Number of Threads")
plt.xticks(sorted(threads.unique()))
plt.grid(True, linestyle=":", alpha=0.6)
plt.legend()
save_plot(plt, RESULTS / "execution_time.png")

# Graph 2: Speedup
plt.figure(figsize=(8, 5))
plt.plot(threads, speedup, marker="o", color="green", linewidth=2, label="Observed Speedup")
plt.plot(threads, threads, linestyle="--", color="gray", linewidth=1.5, label="Ideal Linear (S=p)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup (S)")
plt.title("Speedup vs Number of Threads")
plt.xticks(sorted(threads.unique()))
plt.grid(True, linestyle=":", alpha=0.6)
plt.legend()
save_plot(plt, RESULTS / "speedup.png")

# Graph 3: Parallel Efficiency
plt.figure(figsize=(8, 5))
plt.plot(threads, efficiency, marker="o", color="orange", linewidth=2, label="Observed Efficiency")
plt.axhline(100.0, linestyle="--", color="gray", linewidth=1.5, label="Ideal Efficiency (100%)")
plt.xlabel("Number of Threads")
plt.ylabel("Efficiency (%)")
plt.title("Parallel Efficiency vs Number of Threads")
plt.xticks(sorted(threads.unique()))
plt.grid(True, linestyle=":", alpha=0.6)
plt.legend()
save_plot(plt, RESULTS / "efficiency.png")

print("All three performance graphs generated successfully in results/")

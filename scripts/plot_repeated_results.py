
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

# Project paths
ROOT = Path(__file__).resolve().parent.parent
CSV_FILE = ROOT / "results" / "repeated_benchmark_results.csv"
OUTPUT_DIR = ROOT / "results"

# Read benchmark data
data = pd.read_csv(CSV_FILE)

threads = data["threads"]
serial_time = data["serial_avg_ms"].iloc[0]
parallel_time = data["parallel_avg_ms"]
speedup = data["speedup"]
efficiency = data["efficiency"] * 100

# Graph 1: Execution Time
plt.figure(figsize=(8, 5))
plt.plot(threads, parallel_time, marker="o", label="Parallel")
plt.axhline(serial_time, linestyle="--", label="Serial")
plt.xlabel("Number of Threads")
plt.ylabel("Execution Time (ms)")
plt.title("Execution Time vs Number of Threads")
plt.xticks(threads)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig(OUTPUT_DIR / "execution_time.png", dpi=300)
plt.close()

# Graph 2: Speedup
plt.figure(figsize=(8, 5))
plt.plot(threads, speedup, marker="o")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.title("Speedup vs Number of Threads")
plt.xticks(threads)
plt.grid(True)
plt.tight_layout()
plt.savefig(OUTPUT_DIR / "speedup.png", dpi=300)
plt.close()

# Graph 3: Parallel Efficiency
plt.figure(figsize=(8, 5))
plt.plot(threads, efficiency, marker="o")
plt.xlabel("Number of Threads")
plt.ylabel("Efficiency (%)")
plt.title("Parallel Efficiency vs Number of Threads")
plt.xticks(threads)
plt.grid(True)
plt.tight_layout()
plt.savefig(OUTPUT_DIR / "efficiency.png", dpi=300)
plt.close()

print("All three performance graphs generated successfully!")
print("Saved inside the results folder.")

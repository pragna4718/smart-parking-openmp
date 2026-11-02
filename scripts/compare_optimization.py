
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"

before = pd.read_csv(
    RESULTS / "repeated_benchmark_before_optimization.csv"
)
after = pd.read_csv(
    RESULTS / "repeated_benchmark_after_optimization.csv"
)

threads = before["threads"]

# 1. Execution time comparison
plt.figure(figsize=(8, 5))
plt.plot(threads, before["parallel_avg_ms"], marker="o",
         label="Before Optimization")
plt.plot(threads, after["parallel_avg_ms"], marker="o",
         label="After Optimization")
plt.xlabel("Number of Threads")
plt.ylabel("Execution Time (ms)")
plt.title("Before vs After: Execution Time")
plt.xticks(threads)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig(RESULTS / "optimization_execution_time.png", dpi=300)
plt.close()

# 2. Speedup comparison
plt.figure(figsize=(8, 5))
plt.plot(threads, before["speedup"], marker="o",
         label="Before Optimization")
plt.plot(threads, after["speedup"], marker="o",
         label="After Optimization")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.title("Before vs After: Speedup")
plt.xticks(threads)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig(RESULTS / "optimization_speedup.png", dpi=300)
plt.close()

# 3. Percentage reduction in execution time
reduction = (
    (before["parallel_avg_ms"] - after["parallel_avg_ms"])
    / before["parallel_avg_ms"]
) * 100

plt.figure(figsize=(8, 5))
plt.bar(threads, reduction)
plt.xlabel("Number of Threads")
plt.ylabel("Execution Time Reduction (%)")
plt.title("Performance Improvement After Optimization")
plt.xticks(threads)
plt.grid(axis="y")
plt.tight_layout()
plt.savefig(RESULTS / "optimization_improvement.png", dpi=300)
plt.close()

print("Comparison graphs generated successfully!")
print("Saved inside the results folder.")
print("\nExecution time reduction:")
for t, value in zip(threads, reduction):
    print(f"{t} threads: {value:.2f}%")

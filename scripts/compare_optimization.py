import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from utils import save_plot

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"

before_file = RESULTS / "repeated_benchmark_before_optimization.csv"
after_file = RESULTS / "repeated_benchmark_after_optimization.csv"

if not before_file.exists() or not after_file.exists():
    print(f"[ERROR] Required before/after CSV files not found in {RESULTS}")
    exit(1)

before = pd.read_csv(before_file)
after = pd.read_csv(after_file)

threads = before["threads"]

# 1. Execution time comparison
plt.figure(figsize=(8, 5))
plt.plot(threads, before["parallel_avg_ms"], marker="o", linewidth=2,
         label="Before Optimization (No Precomputation)")
plt.plot(threads, after["parallel_avg_ms"], marker="o", linewidth=2,
         label="After Optimization (With Precomputation)")
plt.xlabel("Number of Threads")
plt.ylabel("Execution Time (ms)")
plt.title("Execution Time: Before vs After Optimization")
plt.xticks(sorted(threads.unique()))
plt.grid(True, linestyle=":", alpha=0.6)
plt.legend()
save_plot(plt, RESULTS / "optimization_execution_time.png")

# 2. Speedup comparison
plt.figure(figsize=(8, 5))
plt.plot(threads, before["speedup"], marker="o", linewidth=2,
         label="Before Optimization Speedup")
plt.plot(threads, after["speedup"], marker="o", linewidth=2,
         label="After Optimization Speedup")
plt.plot(threads, threads, linestyle="--", color="gray", linewidth=1.5, label="Ideal Linear (S=p)")
plt.xlabel("Number of Threads")
plt.ylabel("Speedup (S)")
plt.title("Speedup: Before vs After Optimization")
plt.xticks(sorted(threads.unique()))
plt.grid(True, linestyle=":", alpha=0.6)
plt.legend()
save_plot(plt, RESULTS / "optimization_speedup.png")

# 3. Percentage reduction in execution time
reduction = (
    (before["parallel_avg_ms"] - after["parallel_avg_ms"])
    / before["parallel_avg_ms"]
) * 100

plt.figure(figsize=(8, 5))
bars = plt.bar([str(t) for t in threads], reduction, color="#2ca02c", width=0.55)
plt.xlabel("Number of Threads")
plt.ylabel("Execution Time Reduction (%)")
plt.title("Percentage Performance Improvement After Optimization")
plt.grid(axis="y", linestyle=":", alpha=0.6)
for bar in bars:
    yval = bar.get_height()
    plt.text(bar.get_x() + bar.get_width() / 2.0, yval + 0.5, f"{yval:.1f}%", ha='center', va='bottom', fontweight='bold')
plt.ylim(0, max(reduction) * 1.25)
save_plot(plt, RESULTS / "optimization_improvement.png")

print("Comparison graphs generated successfully in results/")
print("\nExecution time reduction:")
for t, value in zip(threads, reduction):
    print(f"  {t} threads: {value:.2f}%")

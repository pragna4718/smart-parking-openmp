import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from utils import save_plot

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"

df = pd.read_csv(RESULTS / "schedule_comparison.csv")

plt.figure(figsize=(9, 6))

for schedule in df["schedule"].unique():
    data = df[df["schedule"] == schedule]
    plt.plot(
        data["threads"],
        data["parallel_avg_ms"],
        marker="o",
        linewidth=2,
        label=schedule
    )

plt.xlabel("Number of Threads")
plt.ylabel("Average Execution Time (ms)")
plt.title("OpenMP Scheduling Policy Comparison")
plt.xticks(sorted(df["threads"].unique()))
plt.grid(True, linestyle=":", alpha=0.6)
plt.legend()

save_plot(plt, RESULTS / "schedule_comparison.png")
print("Graph saved to results/schedule_comparison.png")

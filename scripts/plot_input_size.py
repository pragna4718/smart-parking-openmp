import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from utils import save_plot

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"

df = pd.read_csv(RESULTS / "input_size_comparison.csv")

plt.figure(figsize=(9, 6))

for workload in df["workload"].unique():
    data = df[df["workload"] == workload]
    plt.plot(
        data["threads"],
        data["parallel_avg_ms"],
        marker="o",
        linewidth=2,
        label=f"{workload} ({data['vehicles'].iloc[0]} veh, {data['slots'].iloc[0]} slots)"
    )

plt.xlabel("Number of Threads")
plt.ylabel("Average Execution Time (ms)")
plt.title("Effect of Input Size on OpenMP Scaling")
plt.xticks(sorted(df["threads"].unique()))
plt.grid(True, linestyle=":", alpha=0.6)
plt.legend()

save_plot(plt, RESULTS / "input_size_comparison.png")
print("Graph saved to results/input_size_comparison.png")

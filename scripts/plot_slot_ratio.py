import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from utils import save_plot

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"

df = pd.read_csv(RESULTS / "slot_ratio_comparison.csv")

plt.figure(figsize=(9, 6))

for config in df["configuration"].unique():
    data = df[df["configuration"] == config]
    ratio = data["slot_vehicle_ratio"].iloc[0]
    plt.plot(
        data["threads"],
        data["parallel_avg_ms"],
        marker="o",
        linewidth=2,
        label=f"{config} (M/N = {ratio})"
    )

plt.xlabel("Number of Threads")
plt.ylabel("Average Execution Time (ms)")
plt.title("Effect of Slot-to-Vehicle Ratio on OpenMP Performance")
plt.xticks(sorted(df["threads"].unique()))
plt.grid(True, linestyle=":", alpha=0.6)
plt.legend()

save_plot(plt, RESULTS / "slot_ratio_comparison.png")
print("Graph saved to results/slot_ratio_comparison.png")

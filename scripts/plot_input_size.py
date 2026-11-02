
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

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
        label=workload
    )

plt.xlabel("Number of Threads")
plt.ylabel("Average Execution Time (ms)")
plt.title("Effect of Input Size on OpenMP Performance")
plt.xticks([1, 2, 4, 8, 10])
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig(
    RESULTS / "input_size_comparison.png",
    dpi=300
)

plt.show()

print("Graph saved to results/input_size_comparison.png")

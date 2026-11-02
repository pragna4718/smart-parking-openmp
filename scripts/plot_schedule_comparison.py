
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"

df = pd.read_csv(RESULTS / "schedule_comparison.csv")

plt.figure(figsize=(10, 6))

for schedule in df["schedule"].unique():
    data = df[df["schedule"] == schedule]
    plt.plot(
        data["threads"],
        data["parallel_avg_ms"],
        marker="o",
        label=schedule
    )

plt.xlabel("Number of Threads")
plt.ylabel("Average Execution Time (ms)")
plt.title("OpenMP Scheduling Policy Comparison")
plt.xticks([1, 2, 4, 8, 10])
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig(
    RESULTS / "schedule_comparison.png",
    dpi=300
)

plt.show()

print("Graph saved to results/schedule_comparison.png")

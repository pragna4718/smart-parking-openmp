
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"

df = pd.read_csv(RESULTS / "slot_ratio_comparison.csv")

plt.figure(figsize=(9, 6))

for config in df["configuration"].unique():
    data = df[df["configuration"] == config]
    plt.plot(
        data["threads"],
        data["parallel_avg_ms"],
        marker="o",
        label=config
    )

plt.xlabel("Number of Threads")
plt.ylabel("Average Execution Time (ms)")
plt.title("Effect of Slot-to-Vehicle Ratio on Performance")
plt.xticks([1, 2, 4, 8, 10])
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig(
    RESULTS / "slot_ratio_comparison.png",
    dpi=300
)

plt.show()
print("Graph saved to results/slot_ratio_comparison.png")

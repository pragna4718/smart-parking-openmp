
import subprocess
import csv
import re
import statistics
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"
BINARY = ROOT / "smart_parking"

THREADS = [1, 2, 4, 8, 10]
REPEATS = 5
VEHICLES = 800

EXPERIMENTS = [
    ("tight", 1, 16, 40, 640, 0.8),
    ("spacious", 1, 24, 40, 960, 1.2),
]

def run_benchmark(floors, rows, cols):
    env = os.environ.copy()
    env["OMP_SCHEDULE"] = "guided"

    command = [
        str(BINARY),
        "--floors", str(floors),
        "--rows", str(rows),
        "--cols", str(cols),
        "--vehicles", str(VEHICLES),
        "--benchmark",
        "--csv", str(RESULTS / "slot_ratio_temp.csv")
    ]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        check=True,
        env=env
    )

    output = result.stdout

    serial_match = re.search(r"Done:\s*([\d.]+)\s*ms", output)
    if not serial_match:
        raise RuntimeError("Serial timing not found")

    serial = float(serial_match.group(1))
    parallel = {}

    for thread in THREADS:
        pattern = rf"^\s*{thread}\s+\d+/\d+\s+([\d.]+)\s+"
        match = re.search(pattern, output, re.MULTILINE)
        if not match:
            raise RuntimeError(f"Missing timing for {thread} threads")
        parallel[thread] = float(match.group(1))

    return serial, parallel


def main():
    RESULTS.mkdir(exist_ok=True)
    output_rows = []

    for name, floors, rows, cols, slots, ratio in EXPERIMENTS:
        print(f"\nTesting {name} configuration: {slots} slots, "
              f"{VEHICLES} vehicles")

        serial_times = []
        parallel_times = {t: [] for t in THREADS}

        for run in range(REPEATS + 1):
            print(f"Run {run + 1}/{REPEATS + 1}")
            serial, parallel = run_benchmark(floors, rows, cols)

            if run == 0:
                continue

            serial_times.append(serial)
            for thread in THREADS:
                parallel_times[thread].append(parallel[thread])

        serial_avg = statistics.mean(serial_times)

        for thread in THREADS:
            parallel_avg = statistics.mean(parallel_times[thread])
            speedup = serial_avg / parallel_avg
            efficiency = speedup / thread

            output_rows.append([
                name, ratio, slots, VEHICLES, thread,
                round(serial_avg, 4),
                round(parallel_avg, 4),
                round(speedup, 4),
                round(efficiency, 4),
                REPEATS
            ])

            print(f"{thread} threads: {parallel_avg:.2f} ms, "
                  f"speedup {speedup:.2f}x")

    output_file = RESULTS / "slot_ratio_comparison.csv"

    with open(output_file, "w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow([
            "configuration", "slot_vehicle_ratio", "slots",
            "vehicles", "threads", "serial_avg_ms",
            "parallel_avg_ms", "speedup", "efficiency", "repeats"
        ])
        writer.writerows(output_rows)

    print(f"\nResults saved to: {output_file}")


if __name__ == "__main__":
    main()

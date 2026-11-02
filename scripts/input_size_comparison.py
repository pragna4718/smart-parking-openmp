
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

EXPERIMENTS = [
    {
        "name": "small",
        "floors": 1,
        "rows": 25,
        "cols": 50,
        "vehicles": 1000,
        "slots": 1250
    },
    {
        "name": "large",
        "floors": 5,
        "rows": 40,
        "cols": 50,
        "vehicles": 8000,
        "slots": 10000
    }
]


def run_benchmark(experiment):
    env = os.environ.copy()
    env["OMP_SCHEDULE"] = "guided"

    command = [
        str(BINARY),
        "--floors", str(experiment["floors"]),
        "--rows", str(experiment["rows"]),
        "--cols", str(experiment["cols"]),
        "--vehicles", str(experiment["vehicles"]),
        "--benchmark",
        "--csv", str(RESULTS / "input_size_temp.csv")
    ]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        check=True,
        env=env
    )

    output = result.stdout

    serial_match = re.search(
        r"Done:\s*([\d.]+)\s*ms", output
    )

    if not serial_match:
        raise RuntimeError("Serial timing not found")

    serial = float(serial_match.group(1))
    parallel = {}

    for thread in THREADS:
        pattern = rf"^\s*{thread}\s+\d+/\d+\s+([\d.]+)\s+"
        match = re.search(pattern, output, re.MULTILINE)

        if not match:
            raise RuntimeError(
                f"Timing not found for {thread} threads"
            )

        parallel[thread] = float(match.group(1))

    return serial, parallel


def main():
    RESULTS.mkdir(exist_ok=True)
    rows = []

    for experiment in EXPERIMENTS:
        print(f"\nTesting {experiment['name']} workload")
        serial_times = []
        parallel_times = {t: [] for t in THREADS}

        for run in range(REPEATS + 1):
            print(f"Run {run + 1}/{REPEATS + 1}")

            serial, parallel = run_benchmark(experiment)

            if run == 0:
                continue

            serial_times.append(serial)

            for thread in THREADS:
                parallel_times[thread].append(
                    parallel[thread]
                )

        serial_avg = statistics.mean(serial_times)

        for thread in THREADS:
            parallel_avg = statistics.mean(
                parallel_times[thread]
            )

            speedup = serial_avg / parallel_avg
            efficiency = speedup / thread

            rows.append([
                experiment["name"],
                experiment["slots"],
                experiment["vehicles"],
                thread,
                round(serial_avg, 4),
                round(parallel_avg, 4),
                round(speedup, 4),
                round(efficiency, 4),
                REPEATS
            ])

            print(
                f"{thread} threads: {parallel_avg:.2f} ms, "
                f"speedup {speedup:.2f}x"
            )

    output_file = RESULTS / "input_size_comparison.csv"

    with open(output_file, "w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow([
            "workload", "slots", "vehicles", "threads",
            "serial_avg_ms", "parallel_avg_ms",
            "speedup", "efficiency", "repeats"
        ])
        writer.writerows(rows)

    print(f"\nResults saved to: {output_file}")


if __name__ == "__main__":
    main()

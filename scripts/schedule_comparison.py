
import subprocess
import csv
import re
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"
BINARY = ROOT / "smart_parking"

THREADS = [1, 2, 4, 8, 10]
REPEATS = 5
VEHICLES = 8000

SCHEDULES = [
    ("static", "static"),
    ("dynamic16", "dynamic,16"),
    ("dynamic64", "dynamic,64"),
    ("dynamic256", "dynamic,256"),
    ("guided", "guided"),
]

def run_one(schedule):
    env = __import__("os").environ.copy()
    env["OMP_SCHEDULE"] = schedule

    command = [
        str(BINARY),
        "--floors", "5",
        "--rows", "40",
        "--cols", "50",
        "--vehicles", str(VEHICLES),
        "--benchmark",
        "--csv", str(RESULTS / "schedule_temp.csv")
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
                f"Missing timing for {thread} threads"
            )

        parallel[thread] = float(match.group(1))

    return serial, parallel


def main():
    RESULTS.mkdir(exist_ok=True)

    output_file = RESULTS / "schedule_comparison.csv"
    rows = []

    for name, schedule in SCHEDULES:
        print(f"\nTesting {name}...")

        serial_times = []
        parallel_times = {t: [] for t in THREADS}

        for run in range(REPEATS + 1):
            print(f"Run {run + 1}/{REPEATS + 1}")

            serial, parallel = run_one(schedule)

            # First run is warm-up
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
                name,
                schedule,
                thread,
                10000,
                VEHICLES,
                round(serial_avg, 4),
                round(parallel_avg, 4),
                round(speedup, 4),
                round(efficiency, 4),
                REPEATS
            ])

            print(
                f"{thread} threads: "
                f"{parallel_avg:.2f} ms, "
                f"speedup {speedup:.2f}x"
            )

    with open(output_file, "w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow([
            "schedule", "omp_schedule", "threads",
            "slots", "vehicles", "serial_avg_ms",
            "parallel_avg_ms", "speedup",
            "efficiency", "repeats"
        ])
        writer.writerows(rows)

    print(f"\nAll results saved to {output_file}")


if __name__ == "__main__":
    main()

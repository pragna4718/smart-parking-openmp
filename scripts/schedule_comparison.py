import subprocess
import csv
import statistics
import os
from pathlib import Path
from utils import get_executable, parse_benchmark_output

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"
BINARY = get_executable(ROOT)

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
    env = os.environ.copy()
    env["OMP_SCHEDULE"] = schedule

    temp_csv = RESULTS / "schedule_temp.csv"
    temp_csv.parent.mkdir(parents=True, exist_ok=True)

    command = [
        str(BINARY),
        "--floors", "5",
        "--rows", "40",
        "--cols", "50",
        "--vehicles", str(VEHICLES),
        "--benchmark",
        "--csv", str(temp_csv)
    ]

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        check=True,
        env=env
    )

    return parse_benchmark_output(result.stdout)

def main():
    RESULTS.mkdir(parents=True, exist_ok=True)
    output_file = RESULTS / "schedule_comparison.csv"
    rows = []
    detected_threads = []

    print(f"Starting schedule comparison benchmark using: {BINARY}")

    for name, schedule in SCHEDULES:
        print(f"\nTesting schedule: {name} (OMP_SCHEDULE={schedule})...")

        serial_times = []
        parallel_times = {}

        for run in range(REPEATS + 1):
            print(f"  Run {run + 1}/{REPEATS + 1}")

            serial, parallel = run_one(schedule)

            if not detected_threads:
                detected_threads = sorted(list(parallel.keys()))

            if not parallel_times:
                parallel_times = {t: [] for t in detected_threads}

            # First run is warm-up
            if run == 0:
                continue

            serial_times.append(serial)
            for thread in detected_threads:
                parallel_times[thread].append(parallel[thread])

        serial_avg = statistics.mean(serial_times)

        for thread in detected_threads:
            parallel_avg = statistics.mean(parallel_times[thread])
            speedup = serial_avg / parallel_avg if parallel_avg > 1e-6 else 0.0
            efficiency = speedup / thread if thread > 0 else 0.0

            rows.append({
                "schedule": name,
                "omp_schedule": schedule,
                "threads": thread,
                "slots": 10000,
                "vehicles": VEHICLES,
                "serial_avg_ms": round(serial_avg, 3),
                "parallel_avg_ms": round(parallel_avg, 3),
                "speedup": round(speedup, 4),
                "efficiency": round(efficiency, 4),
                "repeats": REPEATS
            })

    with open(output_file, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "schedule",
                "omp_schedule",
                "threads",
                "slots",
                "vehicles",
                "serial_avg_ms",
                "parallel_avg_ms",
                "speedup",
                "efficiency",
                "repeats"
            ]
        )
        writer.writeheader()
        writer.writerows(rows)

    print(f"\n[SUCCESS] Schedule comparison saved to: {output_file}")

if __name__ == "__main__":
    main()

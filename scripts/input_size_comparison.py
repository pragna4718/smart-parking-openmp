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

    temp_csv = RESULTS / "input_size_temp.csv"
    temp_csv.parent.mkdir(parents=True, exist_ok=True)

    command = [
        str(BINARY),
        "--floors", str(experiment["floors"]),
        "--rows", str(experiment["rows"]),
        "--cols", str(experiment["cols"]),
        "--vehicles", str(experiment["vehicles"]),
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
    output_file = RESULTS / "input_size_comparison.csv"
    rows = []
    detected_threads = []

    print(f"Starting input size comparison using: {BINARY}")

    for exp in EXPERIMENTS:
        name = exp["name"]
        print(f"\nTesting workload: {name} ({exp['vehicles']} vehicles, {exp['slots']} slots)...")

        serial_times = []
        parallel_times = {}

        for run in range(REPEATS + 1):
            print(f"  Run {run + 1}/{REPEATS + 1}")

            serial, parallel = run_benchmark(exp)

            if not detected_threads:
                detected_threads = sorted(list(parallel.keys()))

            if not parallel_times:
                parallel_times = {t: [] for t in detected_threads}

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
                "workload": name,
                "slots": exp["slots"],
                "vehicles": exp["vehicles"],
                "threads": thread,
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
                "workload",
                "slots",
                "vehicles",
                "threads",
                "serial_avg_ms",
                "parallel_avg_ms",
                "speedup",
                "efficiency",
                "repeats"
            ]
        )
        writer.writeheader()
        writer.writerows(rows)

    print(f"\n[SUCCESS] Input size comparison saved to: {output_file}")

if __name__ == "__main__":
    main()

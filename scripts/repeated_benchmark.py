import subprocess
import csv
import statistics
import os
from pathlib import Path
from utils import get_executable, parse_benchmark_output

# Benchmark configuration
REPEATS = 5
SLOTS = 10000
VEHICLES = 8000

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "results" / "repeated_benchmark_results.csv"

def run_benchmark():
    serial_times = []
    parallel_times = {}
    detected_threads = []

    print("Starting repeated benchmark...")
    executable = get_executable(ROOT)
    print(f"Using binary: {executable}")

    for run in range(REPEATS + 1):
        print(f"\nRun {run + 1}/{REPEATS + 1}")

        temp_csv = ROOT / "results" / "temp.csv"
        temp_csv.parent.mkdir(parents=True, exist_ok=True)

        command = [
            executable,
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
            check=True
        )

        serial_time, measured = parse_benchmark_output(result.stdout)

        if not detected_threads:
            detected_threads = sorted(list(measured.keys()))
            parallel_times = {t: [] for t in detected_threads}

        # First run is a warm-up and is discarded
        if run == 0:
            print("  (Warm-up run discarded)")
            continue

        serial_times.append(serial_time)
        for thread in detected_threads:
            parallel_times[thread].append(measured[thread])

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)

    with open(OUTPUT, "w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow([
            "threads",
            "slots",
            "vehicles",
            "serial_avg_ms",
            "parallel_avg_ms",
            "speedup",
            "efficiency",
            "repeats"
        ])

        serial_avg = statistics.mean(serial_times)

        for thread in detected_threads:
            parallel_avg = statistics.mean(parallel_times[thread])
            speedup = serial_avg / parallel_avg if parallel_avg > 1e-6 else 0.0
            efficiency = speedup / thread if thread > 0 else 0.0

            writer.writerow([
                thread,
                SLOTS,
                VEHICLES,
                round(serial_avg, 3),
                round(parallel_avg, 3),
                round(speedup, 4),
                round(efficiency, 4),
                REPEATS
            ])

    print(f"\n[SUCCESS] Benchmark results saved to: {OUTPUT}")

if __name__ == "__main__":
    run_benchmark()

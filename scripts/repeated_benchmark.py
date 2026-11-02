
import subprocess
import csv
import re
import statistics
from pathlib import Path

# Benchmark configuration
THREADS = [1, 2, 4, 8, 10]
REPEATS = 5

SLOTS = 10000
VEHICLES = 8000

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "results" / "repeated_benchmark_results.csv"

def run_benchmark():
    serial_times = []
    parallel_times = {t: [] for t in THREADS}

    print("Starting repeated benchmark...")
    
    for run in range(REPEATS + 1):
        print(f"\nRun {run + 1}/{REPEATS + 1}")

        command = [
            str(ROOT / "smart_parking"),
            "--floors", "5",
            "--rows", "40",
            "--cols", "50",
            "--vehicles", str(VEHICLES),
            "--benchmark",
            "--csv", str(ROOT / "results" / "temp.csv")
        ]

        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=True
        )

        output = result.stdout

        serial_match = re.search(
            r"Done:\s*([\d.]+)\s*ms",
            output
        )

        if not serial_match:
            raise RuntimeError("Serial time not found in output")

        serial_time = float(serial_match.group(1))

        measured = {}

        for thread in THREADS:
            pattern = (
                rf"^\s*{thread}\s+\d+/\d+\s+"
                rf"([\d.]+)\s+"
            )
            match = re.search(pattern, output, re.MULTILINE)

            if not match:
                raise RuntimeError(
                    f"Result for {thread} threads not found"
                )

            measured[thread] = float(match.group(1))

        # First run is a warm-up and is discarded
        if run == 0:
            continue

        serial_times.append(serial_time)

        for thread in THREADS:
            parallel_times[thread].append(measured[thread])

    OUTPUT.parent.mkdir(exist_ok=True)

    with open(OUTPUT, "w", newline="") as file:
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

        for thread in THREADS:
            parallel_avg = statistics.mean(
                parallel_times[thread]
            )

            speedup = serial_avg / parallel_avg
            efficiency = speedup / thread

            writer.writerow([
                thread,
                SLOTS,
                VEHICLES,
                round(serial_avg, 4),
                round(parallel_avg, 4),
                round(speedup, 4),
                round(efficiency, 4),
                REPEATS
            ])

            print(
                f"{thread} threads: "
                f"{parallel_avg:.3f} ms, "
                f"speedup {speedup:.2f}x"
            )

    print(f"\nResults saved to: {OUTPUT}")

if __name__ == "__main__":
    run_benchmark()

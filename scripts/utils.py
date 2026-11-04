import os
import sys
import re
import platform
import subprocess
from pathlib import Path

def get_executable(root_dir=None):
    if root_dir is None:
        root_dir = Path(__file__).resolve().parent.parent
    else:
        root_dir = Path(root_dir).resolve()

    candidates = [
        root_dir / "smart_parking.exe",
        root_dir / "smart_parking",
        root_dir / "bin" / "smart_parking.exe",
        root_dir / "bin" / "smart_parking"
    ]
    for cand in candidates:
        if cand.is_file():
            return str(cand)

    fallback = "smart_parking.exe" if sys.platform == "win32" else "smart_parking"
    target = root_dir / fallback
    if target.is_file():
        return str(target)
    return str(target)

def parse_benchmark_output(output):
    """
    Dynamically parses the serial time and thread measurements from benchmark stdout.
    Returns (serial_time_ms, {thread: parallel_time_ms})
    """
    serial_match = re.search(r"Done:\s*([\d.]+)\s*ms", output)
    if not serial_match:
        raise RuntimeError("Serial timing not found in benchmark output:\n" + output[:500])

    serial_time = float(serial_match.group(1))
    parallel_times = {}

    for line in output.splitlines():
        # Match table lines: e.g. "       4      73/200         2.68       0.05x           1.3%  YES [PASS]"
        m = re.match(r"^\s*(\d+)\s+\d+/\d+\s+([\d.]+)", line)
        if m:
            t = int(m.group(1))
            ms = float(m.group(2))
            parallel_times[t] = ms

    if not parallel_times:
        raise RuntimeError("No thread timing rows found in output:\n" + output[:500])

    return serial_time, parallel_times

def get_available_threads():
    """Returns detected logical processors to benchmark, e.g. [1, 2, 4, 8, max_t]"""
    try:
        count = os.cpu_count() or 4
    except Exception:
        count = 4

    threads = [1, 2, 4, 8]
    if count >= 10 and 10 not in threads:
        threads.append(10)
    if count not in threads:
        threads.append(count)
    return sorted(list(set(threads)))

def save_plot(plt, output_path, dpi=300):
    """Saves plot to PNG cleanly and closes it without blocking GUI."""
    path = Path(output_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(path, dpi=dpi, bbox_inches="tight")
    # Also save vector PDF if requested or alongside
    pdf_path = path.with_suffix(".pdf")
    try:
        plt.savefig(pdf_path, bbox_inches="tight")
    except Exception:
        pass
    plt.close()

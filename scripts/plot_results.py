import os
import sys
import matplotlib.pyplot as plt

def generate_plots(csv_path="benchmark_results.csv", output_dir="."):
    if not os.path.exists(csv_path):
        print(f"[ERROR] Benchmark CSV file not found at: {csv_path}")
        return False

    threads = []
    serial_times = []
    parallel_times = []
    speedups = []
    efficiencies = []

    with open(csv_path, "r", encoding="utf-8") as f:
        lines = [line.strip() for line in f if line.strip()]

    if len(lines) <= 1:
        print("[ERROR] CSV file is empty or only contains headers.")
        return False

    header = [h.strip() for h in lines[0].split(",")]
    col_map = {col: idx for idx, col in enumerate(header)}
    t_idx = col_map.get("threads", 0)
    s_idx = col_map.get("serial_time_ms", 3)
    p_idx = col_map.get("parallel_time_ms", 4)
    spd_idx = col_map.get("speedup", 5)
    eff_idx = col_map.get("efficiency", 6)

    for line in lines[1:]:
        parts = [p.strip() for p in line.split(",")]
        if len(parts) > max(t_idx, s_idx, p_idx, spd_idx, eff_idx):
            t = int(parts[t_idx])
            s_time = float(parts[s_idx])
            p_time = float(parts[p_idx])
            spd = float(parts[spd_idx])
            eff = float(parts[eff_idx]) * 100.0

            threads.append(t)
            serial_times.append(s_time)
            parallel_times.append(p_time)
            speedups.append(spd)
            efficiencies.append(eff)

    if not threads:
        print("[ERROR] No data parsed from CSV.")
        return False

    # Style configuration
    plt.style.use("seaborn-v0_8-whitegrid" if "seaborn-v0_8-whitegrid" in plt.style.available else "default")
    plt.rcParams["font.sans-serif"] = "DejaVu Sans"
    plt.rcParams["font.family"] = "sans-serif"

    # -------------------------------------------------------------
    # Figure 1: Multi-Panel Performance Dashboard
    # -------------------------------------------------------------
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(18, 5.5))

    # --- Plot 1: Execution Time vs Threads ---
    ax1.plot(threads, parallel_times, marker='o', color='#1f77b4', linewidth=2.5, markersize=8, label='OpenMP Parallel Time')
    ax1.axhline(y=serial_times[0], color='#d62728', linestyle='--', linewidth=2, label=f'Serial Baseline ({serial_times[0]:.1f} ms)')
    ax1.set_title("Execution Time vs Thread Count", fontsize=13, fontweight='bold', pad=10)
    ax1.set_xlabel("Number of Threads (p)", fontsize=11)
    ax1.set_ylabel("Execution Time (milliseconds)", fontsize=11)
    ax1.set_xticks(threads)
    ax1.grid(True, linestyle=":", alpha=0.6)
    ax1.legend(fontsize=10, loc="upper right")

    # Annotate points
    for t, pt in zip(threads, parallel_times):
        ax1.annotate(f"{pt:.1f} ms", (t, pt), textcoords="offset points", xytext=(0, 8), ha='center', fontsize=9)

    # --- Plot 2: Speedup vs Threads ---
    ax2.plot(threads, speedups, marker='s', color='#2ca02c', linewidth=2.5, markersize=8, label='Observed Speedup')
    ax2.plot(threads, threads, linestyle='--', color='#7f7f7f', linewidth=1.8, label='Ideal Linear Speedup (S=p)')
    ax2.set_title("Speedup vs Thread Count", fontsize=13, fontweight='bold', pad=10)
    ax2.set_xlabel("Number of Threads (p)", fontsize=11)
    ax2.set_ylabel("Speedup Ratio S(p) = T_serial / T_par", fontsize=11)
    ax2.set_xticks(threads)
    ax2.grid(True, linestyle=":", alpha=0.6)
    ax2.legend(fontsize=10, loc="upper left")

    for t, sp in zip(threads, speedups):
        ax2.annotate(f"{sp:.2f}x", (t, sp), textcoords="offset points", xytext=(0, 8), ha='center', fontsize=9, fontweight='bold')

    # --- Plot 3: Parallel Efficiency vs Threads ---
    ax3.plot(threads, efficiencies, marker='^', color='#ff7f0e', linewidth=2.5, markersize=8, label='Observed Efficiency')
    ax3.axhline(y=100.0, linestyle='--', color='#7f7f7f', linewidth=1.8, label='Ideal Efficiency (100%)')
    ax3.set_title("Parallel Efficiency vs Thread Count", fontsize=13, fontweight='bold', pad=10)
    ax3.set_xlabel("Number of Threads (p)", fontsize=11)
    ax3.set_ylabel("Efficiency E(p) = S(p)/p (%)", fontsize=11)
    ax3.set_ylim(0, 110)
    ax3.set_xticks(threads)
    ax3.grid(True, linestyle=":", alpha=0.6)
    ax3.legend(fontsize=10, loc="lower left")

    for t, ef in zip(threads, efficiencies):
        ax3.annotate(f"{ef:.1f}%", (t, ef), textcoords="offset points", xytext=(0, 8), ha='center', fontsize=9)

    plt.suptitle("Parallel Smart Parking Allocation (OpenMP Scaling Benchmarks)", fontsize=16, fontweight='bold', y=1.02)
    plt.tight_layout()

    dashboard_path = os.path.join(output_dir, "benchmark_dashboard.png")
    plt.savefig(dashboard_path, dpi=300, bbox_inches="tight")
    plt.close()

    print(f"[SUCCESS] Performance dashboard generated: {dashboard_path}")
    return True

if __name__ == "__main__":
    csv_file = "benchmark_results.csv"
    if len(sys.argv) > 1:
        csv_file = sys.argv[1]
    generate_plots(csv_file)

#!/usr/bin/env python3
"""
Generate comparison charts from NoC benchmark results.

Reads:
  outputs/results/benchmark_results.csv
  outputs/results/synthesis_metrics.csv

Outputs (PNG):
  outputs/report/charts/latency_comparison.png
  outputs/report/charts/bandwidth_comparison.png
  outputs/report/charts/alltoall_throughput.png
  outputs/report/charts/fairness_comparison.png
  outputs/report/charts/area_overhead.png

Run: python3 scripts/generate_charts.py
Requires: pip install matplotlib pandas
"""

import sys
import os
import csv
import re
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')  # headless
    import pandas as pd
except ImportError:
    print("ERROR: Missing dependencies. Run: pip install matplotlib pandas")
    sys.exit(1)

REPO = Path(__file__).parent.parent
RESULTS = REPO / "outputs" / "results"
CHARTS = REPO / "outputs" / "report" / "charts"

CONFIGS = ["SharedBusSoC", "ThesisSoC", "QuadCoreRing", "QuadCoreMesh", "QuadCoreTree"]
CONFIG_LABELS = {
    "SharedBusSoC": "Shared Bus",
    "ThesisSoC": "Crossbar",
    "QuadCoreRing": "Ring NoC",
    "QuadCoreMesh": "Mesh NoC",
    "QuadCoreTree": "Tree NoC",
}
COLORS = ["#d62728", "#ff7f0e", "#2ca02c", "#1f77b4", "#9467bd"]


def extract_cycles(line: str) -> float | None:
    """Extract a cycle count from a benchmark output line."""
    m = re.search(r'(\d[\d,]*)\s+cycles', line.replace(',', ''))
    if m:
        return float(m.group(1))
    return None


def load_benchmark_results() -> dict:
    """Load benchmark_results.csv into {config: {benchmark: [lines]}}."""
    path = RESULTS / "benchmark_results.csv"
    if not path.exists():
        print(f"WARNING: {path} not found — no simulation data yet")
        return {}
    data: dict = {}
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            cfg = row["config"]
            bench = row["benchmark"]
            line = row["line"]
            data.setdefault(cfg, {}).setdefault(bench, []).append(line)
    return data


def load_synthesis_metrics() -> pd.DataFrame | None:
    """Load synthesis_metrics.csv."""
    path = RESULTS / "synthesis_metrics.csv"
    if not path.exists():
        print(f"WARNING: {path} not found — no synthesis data yet")
        return None
    return pd.read_csv(path)


def bar_chart(ax, values: dict, title: str, ylabel: str):
    """Draw a grouped bar chart. values = {config: value}."""
    cfgs = [c for c in CONFIGS if c in values]
    vals = [values[c] for c in cfgs]
    labels = [CONFIG_LABELS.get(c, c) for c in cfgs]
    colors = [COLORS[CONFIGS.index(c)] for c in cfgs]
    bars = ax.bar(labels, vals, color=colors, edgecolor='black', linewidth=0.5)
    ax.set_title(title, fontsize=11, fontweight='bold')
    ax.set_ylabel(ylabel)
    ax.tick_params(axis='x', rotation=15)
    for bar, val in zip(bars, vals):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() * 1.01,
                f'{val:.0f}', ha='center', va='bottom', fontsize=8)


def make_latency_chart(data: dict):
    """bench_pingpong: Core 0 <-> Core 1 latency."""
    values = {}
    for cfg in CONFIGS:
        lines = data.get(cfg, {}).get("bench_pingpong", [])
        for line in lines:
            if "Core 0 <-> Core 1" in line:
                cyc = extract_cycles(line)
                if cyc:
                    values[cfg] = cyc
                    break
    if not values:
        print("SKIP: latency chart (no bench_pingpong data)")
        return
    fig, ax = plt.subplots(figsize=(8, 5))
    bar_chart(ax, values, "Core-to-Core Ping-Pong Latency (Core 0 ↔ Core 1)", "Cycles/round-trip")
    fig.tight_layout()
    fig.savefig(CHARTS / "latency_comparison.png", dpi=150)
    plt.close(fig)
    print("Saved: latency_comparison.png")


def make_bandwidth_chart(data: dict):
    """bench_mem_bandwidth: peak bandwidth."""
    values = {}
    for cfg in CONFIGS:
        lines = data.get(cfg, {}).get("bench_mem_bandwidth", [])
        for line in lines:
            if "bytes" in line.lower() and "cycle" in line.lower():
                m = re.search(r'(\d+)\s+bytes/cycle', line)
                if m:
                    values[cfg] = float(m.group(1))
                    break
    if not values:
        print("SKIP: bandwidth chart (no bench_mem_bandwidth data)")
        return
    fig, ax = plt.subplots(figsize=(8, 5))
    bar_chart(ax, values, "Memory Bandwidth", "bytes/cycle")
    fig.tight_layout()
    fig.savefig(CHARTS / "bandwidth_comparison.png", dpi=150)
    plt.close(fig)
    print("Saved: bandwidth_comparison.png")


def make_alltoall_chart(data: dict):
    """bench_alltoall: aggregate throughput."""
    values = {}
    for cfg in CONFIGS:
        lines = data.get(cfg, {}).get("bench_alltoall", [])
        for line in lines:
            if "Throughput" in line:
                m = re.search(r'(\d+)\s+bytes/cycle', line)
                if m:
                    values[cfg] = float(m.group(1)) / 1000.0  # undo x1000 scaling
                    break
    if not values:
        print("SKIP: alltoall chart (no bench_alltoall data)")
        return
    fig, ax = plt.subplots(figsize=(8, 5))
    bar_chart(ax, values, "All-to-All Aggregate Throughput", "bytes/cycle")
    fig.tight_layout()
    fig.savefig(CHARTS / "alltoall_throughput.png", dpi=150)
    plt.close(fig)
    print("Saved: alltoall_throughput.png")


def make_fairness_chart(data: dict):
    """bench_alltoall + bench_hotspot: fairness (max/min ratio, lower=better)."""
    values = {}
    for cfg in CONFIGS:
        lines = data.get(cfg, {}).get("bench_hotspot", [])
        for line in lines:
            if "Max/Min" in line:
                m = re.search(r'(\d+\.\d+)x', line)
                if m:
                    values[cfg] = float(m.group(1))
                    break
    if not values:
        print("SKIP: fairness chart (no bench_hotspot data)")
        return
    fig, ax = plt.subplots(figsize=(8, 5))
    bar_chart(ax, values, "Hot-Spot Contention Fairness (Max/Min, lower=fairer)", "Max/Min ratio")
    fig.tight_layout()
    fig.savefig(CHARTS / "fairness_comparison.png", dpi=150)
    plt.close(fig)
    print("Saved: fairness_comparison.png")


def make_area_chart(df):
    """Synthesis: LUT usage per config."""
    if df is None:
        print("SKIP: area chart (no synthesis data)")
        return
    df_clean = df.dropna(subset=["lut_used"])
    if df_clean.empty:
        print("SKIP: area chart (lut_used all NaN)")
        return
    values = {row["config"]: float(str(row["lut_used"]).replace(",", ""))
              for _, row in df_clean.iterrows()
              if str(row["lut_used"]) not in ("-", "nan")}
    if not values:
        print("SKIP: area chart (no numeric lut_used)")
        return
    fig, ax = plt.subplots(figsize=(8, 5))
    bar_chart(ax, values, "FPGA Area: LUT Utilization per Config", "LUTs used")
    fig.tight_layout()
    fig.savefig(CHARTS / "area_overhead.png", dpi=150)
    plt.close(fig)
    print("Saved: area_overhead.png")


def main():
    CHARTS.mkdir(parents=True, exist_ok=True)
    data = load_benchmark_results()
    df = load_synthesis_metrics()

    make_latency_chart(data)
    make_bandwidth_chart(data)
    make_alltoall_chart(data)
    make_fairness_chart(data)
    make_area_chart(df)

    print("Done. Charts saved to:", CHARTS)


if __name__ == "__main__":
    main()

#!/bin/bash
# Extract key metrics from Vivado synthesis reports into CSV.
# Run from repo root after collecting reports to outputs/fpga/.
#
# Output: outputs/results/synthesis_metrics.csv

set -euo pipefail

CSV="outputs/results/synthesis_metrics.csv"
mkdir -p outputs/results

echo "config,lut_used,lut_total,ff_used,ff_total,bram_used,bram_total,wns_ns,power_w" > "${CSV}"

for cfg_dir in outputs/fpga/*/; do
    [ -d "$cfg_dir" ] || continue
    cfg=$(basename "$cfg_dir")
    util="${cfg_dir}utilization.rpt"
    timing="${cfg_dir}timing.rpt"
    power="${cfg_dir}power.rpt"

    lut_used="-"; lut_total="-"
    ff_used="-"; ff_total="-"
    bram_used="-"; bram_total="-"
    wns="-"; pwr="-"

    if [ -f "$util" ]; then
        lut_used=$(grep -m1 "CLB LUTs" "$util" | awk '{print $4}' || echo "-")
        lut_total=$(grep -m1 "CLB LUTs" "$util" | awk '{print $6}' || echo "-")
        ff_used=$(grep -m1 "CLB Registers" "$util" | awk '{print $4}' || echo "-")
        ff_total=$(grep -m1 "CLB Registers" "$util" | awk '{print $6}' || echo "-")
        bram_used=$(grep -m1 "Block RAM Tile" "$util" | awk '{print $5}' || echo "-")
        bram_total=$(grep -m1 "Block RAM Tile" "$util" | awk '{print $7}' || echo "-")
    fi
    if [ -f "$timing" ]; then
        wns=$(grep "WNS" "$timing" | head -1 | awk '{print $2}' || echo "-")
    fi
    if [ -f "$power" ]; then
        pwr=$(grep "Total On-Chip Power" "$power" | awk '{print $5}' || echo "-")
    fi

    echo "${cfg},${lut_used},${lut_total},${ff_used},${ff_total},${bram_used},${bram_total},${wns},${pwr}" >> "${CSV}"
    echo "  Processed: ${cfg}"
done

echo "Synthesis metrics written to: ${CSV}"

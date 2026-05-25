#!/bin/bash
# Parse all benchmark logs into a single CSV.
# Run from the repo root after simulations complete.
#
# Output: outputs/results/benchmark_results.csv
# Format: config,benchmark,metric,value

set -euo pipefail

OUTDIR="outputs/sim"
RESULTS_DIR="outputs/results"
CSV="${RESULTS_DIR}/benchmark_results.csv"

mkdir -p "$RESULTS_DIR"
echo "config,benchmark,line" > "$CSV"

configs_found=0
logs_found=0

for cfg_dir in "${OUTDIR}"/*/; do
    [ -d "$cfg_dir" ] || continue
    cfg=$(basename "$cfg_dir")
    configs_found=$((configs_found + 1))

    for log in "${cfg_dir}"*.log; do
        [ -f "$log" ] || continue
        bench=$(basename "$log" .log)
        logs_found=$((logs_found + 1))

        # Extract key output lines from UART log
        grep -E "cycles|bytes|Fairness|Spread|Throughput|speedup|round-trip" "$log" 2>/dev/null | while IFS= read -r line; do
            # Escape any commas in the line content
            escaped=$(printf '%s' "$line" | sed 's/,/;/g')
            printf '%s,%s,"%s"\n' "$cfg" "$bench" "$escaped" >> "$CSV"
        done
    done
done

echo "Configs found: ${configs_found}"
echo "Logs processed: ${logs_found}"
echo "Results written to: ${CSV}"

#!/bin/bash
# Interactive FPGA demo script for NoC comparison.
# Programs VC707 with a selected config, runs benchmarks, and displays results.
#
# Requirements:
#   - Vivado 2021.2 in PATH (for programming)
#   - picocom installed (for UART capture)
#   - VC707 connected via USB-JTAG and USB-UART

set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

declare -A BITSTREAMS=(
    [1]="SharedBusVC707:outputs/fpga/SharedBusVC707/bitstream.bit"
    [2]="CrossbarVC707:outputs/fpga/CrossbarVC707/bitstream.bit"
    [3]="RingVC707:outputs/fpga/RingVC707/bitstream.bit"
    [4]="MeshVC707:outputs/fpga/MeshVC707/bitstream.bit"
    [5]="TreeVC707:outputs/fpga/TreeVC707/bitstream.bit"
)

BENCHMARKS=(bench_pingpong bench_alltoall bench_hotspot bench_streaming bench_mem_latency bench_mem_bandwidth bench_matmul_scale)
UART_DEV="${UART_DEV:-/dev/ttyUSB0}"
UART_BAUD="${UART_BAUD:-57600}"

echo "============================================"
echo " NoC FPGA Demo — VC707 Interactive Runner"
echo "============================================"
echo ""
echo "Select config to program:"
for key in $(echo "${!BITSTREAMS[@]}" | tr ' ' '\n' | sort -n); do
    name="${BITSTREAMS[$key]%%:*}"
    bit="${BITSTREAMS[$key]#*:}"
    status="[NOT FOUND]"
    [ -f "$bit" ] && status="[ready]"
    echo "  ${key}) ${name}  ${status}"
done
echo ""
read -rp "Enter number (1-5): " choice

if [ -z "${BITSTREAMS[$choice]+x}" ]; then
    echo "Invalid choice."
    exit 1
fi

cfg_name="${BITSTREAMS[$choice]%%:*}"
bit_path="${BITSTREAMS[$choice]#*:}"

if [ ! -f "$bit_path" ]; then
    echo "ERROR: Bitstream not found: ${bit_path}"
    echo "Run ./scripts/run_fpga_synthesis.sh first."
    exit 1
fi

echo ""
echo "Programming VC707 with: ${cfg_name}"
echo "Bitstream: ${bit_path}"
echo ""

# Program via Vivado batch
cat > /tmp/program_vc707.tcl << EOF
open_hw_manager
connect_hw_server
open_hw_target
set_property PROGRAM.FILE {${bit_path}} [get_hw_devices xc7vx485t_0]
program_hw_devices [get_hw_devices xc7vx485t_0]
close_hw_manager
EOF

vivado -mode batch -source /tmp/program_vc707.tcl -nojournal -nolog
echo "Programming complete."
echo ""

# Run benchmarks via UART
mkdir -p "outputs/fpga/${cfg_name}"
echo "UART device: ${UART_DEV} @ ${UART_BAUD} baud"
echo ""
echo "Copy benchmark ELFs to SD card, then press Enter to start capture..."
read -rp ""

for bench in "${BENCHMARKS[@]}"; do
    out_log="outputs/fpga/${cfg_name}/${bench}_fpga.log"
    echo "--- Running ${bench} ---"
    echo "  Load: demoriscv/software/Benchmark/build/${bench}.elf → SD card → boot"
    echo "  Press Enter when board is booting this benchmark..."
    read -rp ""
    echo "  Capturing UART output (Ctrl-C to stop and move to next benchmark)..."
    timeout 120 picocom --baud "${UART_BAUD}" --noreset "${UART_DEV}" \
        | tee "${out_log}" || true
    echo "  Saved: ${out_log}"
    echo ""
done

echo "=== Demo complete for ${cfg_name} ==="
echo "Logs in: outputs/fpga/${cfg_name}/"
echo ""
echo "Compare results with simulation:"
grep -h "cycles\|Throughput\|Fairness\|Spread" \
    outputs/fpga/${cfg_name}/*_fpga.log 2>/dev/null || echo "(no data yet)"

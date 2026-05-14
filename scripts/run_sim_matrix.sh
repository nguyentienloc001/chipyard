#!/bin/bash
# Run all 5 configs x 7 benchmarks via parallel git worktrees.
# Each config gets its own Verilator worktree to avoid build conflicts.
#
# Usage: ./scripts/run_sim_matrix.sh
# Run from repo root. Requires RISCV env set (source env.sh first).

set -euo pipefail

REPO=$(git rev-parse --show-toplevel)
WTBASE=/mnt/data/chipyard-worktrees
BRANCH=$(git rev-parse --abbrev-ref HEAD)

CONFIGS=(SharedBusSoC ThesisSoC QuadCoreRing QuadCoreMesh QuadCoreTree)
BENCHMARKS=(bench_mem_latency bench_mem_bandwidth bench_matmul_scale bench_pingpong bench_alltoall bench_hotspot bench_streaming)

echo "=== NoC Simulation Matrix Setup ==="
echo "Branch: ${BRANCH}"
echo "Configs: ${CONFIGS[*]}"
echo "Benchmarks: ${BENCHMARKS[*]}"
echo ""

# Step 1: Build benchmark firmware
echo "--- Building firmware ---"
cd "${REPO}/demoriscv/software/Benchmark"
make clean all
mkdir -p "${REPO}/outputs/fw"
cp build/*.elf "${REPO}/outputs/fw/"
echo "Firmware built: $(ls "${REPO}/outputs/fw/"*.elf | wc -l) ELFs"
cd "${REPO}"

# Step 2: Set up worktrees
echo "--- Setting up simulation worktrees ---"
mkdir -p "${WTBASE}"
for cfg in "${CONFIGS[@]}"; do
    wt="${WTBASE}/sim-${cfg}"
    if [ ! -d "${wt}" ]; then
        echo "  Creating worktree: ${wt}"
        git worktree add "${wt}" "${BRANCH}"
    else
        echo "  Worktree exists: ${wt}"
    fi
done

# Step 3: Print per-config run commands
echo ""
echo "=== Run each of these in a separate terminal (parallel): ==="
for cfg in "${CONFIGS[@]}"; do
    wt="${WTBASE}/sim-${cfg}"
    cat << EOF

# --- ${cfg} ---
cd "${wt}/sims/verilator"
source ../../env.sh
make SUB_PROJECT=chipyard CONFIG=${cfg}
mkdir -p ../../outputs/sim/${cfg}
for bench in ${BENCHMARKS[*]}; do
  echo "=== ${cfg} / \${bench} ==="
  make SUB_PROJECT=chipyard CONFIG=${cfg} \\
    BINARY=../../demoriscv/software/Benchmark/build/\${bench}.elf \\
    TIMEOUT_CYCLES=500000000 run-binary 2>&1 | tee ../../outputs/sim/${cfg}/\${bench}.log || true
done
EOF
done

echo ""
echo "=== After all simulations complete, collect results: ==="
cat << 'EOF'
for cfg in SharedBusSoC ThesisSoC QuadCoreRing QuadCoreMesh QuadCoreTree; do
  wt="/mnt/data/chipyard-worktrees/sim-${cfg}"
  mkdir -p "outputs/sim/${cfg}"
  cp "${wt}/outputs/sim/${cfg}/"*.log "outputs/sim/${cfg}/" 2>/dev/null || true
done
./scripts/parse_bench_results.sh
EOF

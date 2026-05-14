#!/bin/bash
# Run Vivado synthesis for all 5 VC707 configs via parallel git worktrees.
# Each config gets its own worktree to avoid generated-src conflicts.
#
# Usage: ./scripts/run_fpga_synthesis.sh
# Requires Vivado 2021.2 in PATH. Each synthesis takes 2-8 hours.

set -euo pipefail

REPO=$(git rev-parse --show-toplevel)
WTBASE=/mnt/data/chipyard-worktrees
BRANCH=$(git rev-parse --abbrev-ref HEAD)

declare -A FPGA_CONFIGS=(
    [SharedBusVC707]="SharedBusVC707Config"
    [CrossbarVC707]="QuadCoreXBarVC707Config"
    [RingVC707]="QuadCoreNoCVC707Config"
    [MeshVC707]="QuadCoreMeshVC707Config"
    [TreeVC707]="QuadCoreTreeVC707Config"
)

echo "=== VC707 FPGA Synthesis Setup ==="
echo "Branch: ${BRANCH}"
echo ""
echo "WARNING: Each synthesis takes 2-8 hours."
echo "WARNING: Each Vivado instance uses 8-16GB RAM."
echo "WARNING: Requires Vivado 2021.2 license for each parallel run."
echo ""

# Step 1: Set up worktrees
echo "--- Setting up FPGA synthesis worktrees ---"
mkdir -p "${WTBASE}"
for name in "${!FPGA_CONFIGS[@]}"; do
    wt="${WTBASE}/fpga-${name}"
    if [ ! -d "${wt}" ]; then
        echo "  Creating worktree: ${wt}"
        git worktree add "${wt}" "${BRANCH}"
    else
        echo "  Worktree exists: ${wt}"
    fi
done

# Step 2: Print per-config synthesis commands
echo ""
echo "=== Run each of these in a separate terminal (parallel, license-limited): ==="
for name in "${!FPGA_CONFIGS[@]}"; do
    vivado_cfg="${FPGA_CONFIGS[$name]}"
    wt="${WTBASE}/fpga-${name}"
    cat << EOF

# --- ${name} (${vivado_cfg}) ---
cd "${wt}/demoriscv"
source ../env.sh
make SUB_PROJECT=vc707 CONFIG=${vivado_cfg} bitstream 2>&1 | tee ../vivado_${name}.log
EOF
done

echo ""
echo "=== After synthesis completes, collect artifacts: ==="
cat << 'EOF'
WTBASE=/mnt/data/chipyard-worktrees
REPO=$(git rev-parse --show-toplevel)
for name in SharedBusVC707 CrossbarVC707 RingVC707 MeshVC707 TreeVC707; do
  wt="${WTBASE}/fpga-${name}"
  outdir="${REPO}/outputs/fpga/${name}"
  mkdir -p "${outdir}"
  find "${wt}/demoriscv/fpga" -name "*.bit" -exec cp {} "${outdir}/bitstream.bit" \; 2>/dev/null || true
  find "${wt}/demoriscv/fpga" -name "*utilization*.rpt" -exec cp {} "${outdir}/utilization.rpt" \; 2>/dev/null || true
  find "${wt}/demoriscv/fpga" -name "*timing*.rpt" -exec cp {} "${outdir}/timing.rpt" \; 2>/dev/null || true
  find "${wt}/demoriscv/fpga" -name "*power*.rpt" -exec cp {} "${outdir}/power.rpt" \; 2>/dev/null || true
done
# Clean up worktrees after collection
for name in SharedBusVC707 CrossbarVC707 RingVC707 MeshVC707 TreeVC707; do
  git worktree remove "${WTBASE}/fpga-${name}" --force 2>/dev/null || true
done
EOF

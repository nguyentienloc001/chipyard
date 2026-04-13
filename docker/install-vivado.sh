#!/bin/bash
set -euo pipefail

#############################################################################
# Install Vivado to vivado-install/ for Docker image inclusion.
#
# Accepts either a pre-extracted directory or a .tar.gz tarball.
# When a tarball is given, it is extracted to /tmp (needs ~72 GB on the
# root filesystem) and cleaned up automatically afterwards.
#
# Usage:
#   ./docker/install-vivado.sh                          # auto-detect
#   ./docker/install-vivado.sh Xilinx_Unified_2021.2_1021_0703.tar.gz
#   ./docker/install-vivado.sh /path/to/Xilinx_Unified_2021.2_1021_0703/
#
# Disk requirements:
#   /tmp (or root fs): ~72 GB for tarball extraction (freed after install)
#   vivado-install/  : ~8-10 GB for the installed result (Virtex-7 only, pruned)
#############################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

CONFIG_FILE="${SCRIPT_DIR}/install_config.txt"
INSTALL_DIR="${PROJECT_ROOT}/vivado-install"

# ── Resolve installer path ────────────────────────────────────────────────
TARBALL=""
INSTALLER_DIR=""
EXTRACT_TMP=""          # set if we extracted; cleaned up on exit

cleanup() {
    if [ -n "$EXTRACT_TMP" ] && [ -d "$EXTRACT_TMP" ]; then
        echo "Cleaning up extraction at ${EXTRACT_TMP}..."
        rm -rf "$EXTRACT_TMP"
    fi
}
trap cleanup EXIT

if [ "${1:-}" != "" ]; then
    INPUT="$1"
    if [[ "$INPUT" == *.tar.gz || "$INPUT" == *.tgz ]]; then
        TARBALL="$INPUT"
    else
        INSTALLER_DIR="$INPUT"
    fi
else
    # Auto-detect: prefer already-extracted dir, fall back to tarball
    if [ -d "${PROJECT_ROOT}/Xilinx_Unified_2021.2_1021_0703" ]; then
        INSTALLER_DIR="${PROJECT_ROOT}/Xilinx_Unified_2021.2_1021_0703"
    elif [ -f "${PROJECT_ROOT}/Xilinx_Unified_2021.2_1021_0703.tar.gz" ]; then
        TARBALL="${PROJECT_ROOT}/Xilinx_Unified_2021.2_1021_0703.tar.gz"
    fi
fi

# ── Extract tarball if needed ─────────────────────────────────────────────
if [ -n "$TARBALL" ]; then
    if [ ! -f "$TARBALL" ]; then
        echo "ERROR: Tarball not found: ${TARBALL}"
        exit 1
    fi
    TARBALL_SIZE=$(du -sh "$TARBALL" | cut -f1)
    EXTRACT_TMP="/tmp/vivado-installer-$$"
    mkdir -p "$EXTRACT_TMP"

    echo "═══════════════════════════════════════════════════════════════"
    echo "  Extracting Vivado installer tarball"
    echo "  Source:  ${TARBALL} (${TARBALL_SIZE})"
    echo "  Target:  ${EXTRACT_TMP}"
    echo "  Note:    ~72 GB needed on root filesystem; freed after install"
    echo "═══════════════════════════════════════════════════════════════"
    echo ""

    # Check root fs has enough room
    ROOT_FREE_GB=$(df --output=avail -BG / | tail -1 | tr -d 'G ')
    if [ "${ROOT_FREE_GB}" -lt 75 ]; then
        echo "ERROR: Not enough free space on / (${ROOT_FREE_GB} GB free, need ~75 GB)"
        echo "       Free up space on the root filesystem before proceeding."
        exit 1
    fi

    tar -xzf "$TARBALL" -C "$EXTRACT_TMP"
    INSTALLER_DIR="${EXTRACT_TMP}/Xilinx_Unified_2021.2_1021_0703"
fi

# ── Validate installer ────────────────────────────────────────────────────
if [ -z "$INSTALLER_DIR" ]; then
    echo "ERROR: No installer found."
    echo "       Provide the tarball or extracted directory as an argument."
    echo "       Example: $0 Xilinx_Unified_2021.2_1021_0703.tar.gz"
    exit 1
fi

if [ ! -f "${INSTALLER_DIR}/xsetup" ]; then
    echo "ERROR: xsetup not found at ${INSTALLER_DIR}/xsetup"
    exit 1
fi

if [ ! -f "${CONFIG_FILE}" ]; then
    echo "ERROR: Install config not found at ${CONFIG_FILE}"
    exit 1
fi

echo "═══════════════════════════════════════════════════════════════"
echo "  Vivado Host Installation (for Docker)"
echo "═══════════════════════════════════════════════════════════════"
echo "  Installer:    ${INSTALLER_DIR}"
echo "  Destination:  ${INSTALL_DIR}"
echo "  Config:       ${CONFIG_FILE}"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Check /mnt/data has room for the install result
DATA_FREE_GB=$(df --output=avail -BG "${PROJECT_ROOT}" | tail -1 | tr -d 'G ')
if [ "${DATA_FREE_GB}" -lt 25 ]; then
    echo "ERROR: Not enough free space in ${PROJECT_ROOT} (${DATA_FREE_GB} GB free, need ~25 GB)"
    exit 1
fi

# Create a temporary config with the correct install destination
TMP_CONFIG=$(mktemp)
sed "s|^Destination=.*|Destination=${INSTALL_DIR}|" "${CONFIG_FILE}" > "${TMP_CONFIG}"

echo "Running Vivado batch install (this may take 30-60 minutes)..."
"${INSTALLER_DIR}/xsetup" \
    -a XilinxEULA,3rdPartyEULA \
    -b Install \
    -c "${TMP_CONFIG}"
XSETUP_EXIT=$?

rm -f "${TMP_CONFIG}"

# Print installer log for diagnostics
XINSTALL_LOG=$(ls "$HOME"/.Xilinx/xinstall/*.log 2>/dev/null | tail -1 || true)
if [ -n "$XINSTALL_LOG" ]; then
    echo ""
    echo "=== Installer log: ${XINSTALL_LOG} ==="
    cat "$XINSTALL_LOG"
fi

# Hard-fail if Vivado wasn't actually installed (xsetup exits 0 even on error)
if [ ! -d "${INSTALL_DIR}/Vivado/2021.2" ]; then
    echo ""
    echo "ERROR: Vivado was not installed to ${INSTALL_DIR}/Vivado/2021.2"
    echo "       Check the installer log above for details."
    exit 1
fi

echo ""
echo "Cleaning up unnecessary files to reduce Docker image size..."
VIVADO="${INSTALL_DIR}/Vivado/2021.2"

# Basic cleanup
rm -rf "${VIVADO}/doc"      2>/dev/null || true
rm -rf "${VIVADO}/examples" 2>/dev/null || true
rm -rf "${INSTALL_DIR}/.xinstall" 2>/dev/null || true
find "${INSTALL_DIR}" -name "*.debug" -delete 2>/dev/null || true
find "${INSTALL_DIR}" -name "*.log"   -delete 2>/dev/null || true

# ── Large unnecessary directories (~52 GB total) ──────────────────────────
#
# data/parts/xilinx/devint/vault/versal  ~39 GB  Versal AI device timing data
#   (we only target Virtex-7 / xc7vx485t for VC707)
echo "  Removing Versal device data (~39 GB)..."
rm -rf "${VIVADO}/data/parts/xilinx/devint/vault/versal" 2>/dev/null || true

# data/xsim        ~3.5 GB  Vivado simulator — not needed for FPGA synthesis
# data/secureip    ~1.9 GB  Encrypted simulation models
# data/deca        ~1.6 GB  ML-based timing prediction models
# data/resource_est ~1.1 GB Resource estimator ML models
# data/simmodels   ~424 MB  Simulation primitive models
echo "  Removing simulation/estimation data (~8 GB)..."
rm -rf "${VIVADO}/data/xsim"         2>/dev/null || true
rm -rf "${VIVADO}/data/secureip"     2>/dev/null || true
rm -rf "${VIVADO}/data/deca"         2>/dev/null || true
rm -rf "${VIVADO}/data/resource_est" 2>/dev/null || true
rm -rf "${VIVADO}/data/simmodels"    2>/dev/null || true

# ids_lite    ~2.6 GB  Old ISE Design Suite leftovers
# gnu/microblaze ~1.4 GB  MicroBlaze toolchain (not needed for RISC-V/Rocket)
echo "  Removing ISE/MicroBlaze leftovers (~4 GB)..."
rm -rf "${VIVADO}/ids_lite"          2>/dev/null || true
rm -rf "${VIVADO}/gnu/microblaze"    2>/dev/null || true

INSTALL_SIZE=$(du -sh "${INSTALL_DIR}" | cut -f1)

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  Vivado installed successfully!"
echo "  Location: ${INSTALL_DIR}"
echo "  Size:     ${INSTALL_SIZE}"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Next step: build the Docker image"
echo "  cd ${PROJECT_ROOT}"
echo "  ./docker/build.sh"

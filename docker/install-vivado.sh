#!/bin/bash
set -euo pipefail

#############################################################################
# Install Vivado to a local directory for Docker image inclusion.
#
# This avoids COPY-ing the 73GB installer into the Docker build context.
# Instead, Vivado is installed on the host, cleaned up, then the Docker
# build COPY-s only the installed result (~15-20GB).
#
# Usage:
#   ./docker/install-vivado.sh [installer_dir]
#
# Default installer dir: Xilinx_Unified_2021.2_1021_0703/
#############################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

INSTALLER_DIR="${1:-${PROJECT_ROOT}/Xilinx_Unified_2021.2_1021_0703}"
INSTALL_DIR="${PROJECT_ROOT}/vivado-install"
CONFIG_FILE="${SCRIPT_DIR}/install_config.txt"

if [ ! -f "${INSTALLER_DIR}/xsetup" ]; then
    echo "ERROR: Installer not found at ${INSTALLER_DIR}/xsetup"
    echo "Usage: $0 [path/to/Xilinx_Unified_2021.2_1021_0703]"
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

# Create a temporary config with the correct install destination
TMP_CONFIG=$(mktemp)
sed "s|^Destination=.*|Destination=${INSTALL_DIR}|" "${CONFIG_FILE}" > "${TMP_CONFIG}"

echo "Running Vivado batch install (this may take 30-60 minutes)..."
"${INSTALLER_DIR}/xsetup" \
    -a XilinxEULA,3rdPartyEULA \
    -b Install \
    -c "${TMP_CONFIG}"

rm -f "${TMP_CONFIG}"

echo ""
echo "Cleaning up unnecessary files..."

# Remove docs, examples, debug symbols, logs
rm -rf "${INSTALL_DIR}/Vivado/2021.2/doc" 2>/dev/null || true
rm -rf "${INSTALL_DIR}/Vivado/2021.2/examples" 2>/dev/null || true
rm -rf "${INSTALL_DIR}/.xinstall" 2>/dev/null || true
find "${INSTALL_DIR}" -name "*.debug" -delete 2>/dev/null || true
find "${INSTALL_DIR}" -name "*.log" -delete 2>/dev/null || true

INSTALL_SIZE=$(du -sh "${INSTALL_DIR}" | cut -f1)

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  Vivado installed successfully!"
echo "  Location: ${INSTALL_DIR}"
echo "  Size:     ${INSTALL_SIZE}"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Next: run docker build to include Vivado in the image:"
echo "  docker build -f docker/Dockerfile -t locnguyen96/chipyard-dev ."

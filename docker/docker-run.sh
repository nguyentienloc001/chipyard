#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
IMAGE_NAME="locnguyen96/chipyard-dev:latest"

# Use -it when TTY is available, -i only otherwise (for Makefile/CI)
DOCKER_TTY_FLAG="-i"
[ -t 0 ] && DOCKER_TTY_FLAG="-it"

echo "=== Chipyard Docker Helper ==="
echo ""

case "${1:-shell}" in
    build)
        echo "Use docker/build.sh to build the image."
        echo "  ./docker/build.sh --platform linux/arm64"
        echo "  ./docker/build.sh --push"
        exit 0
        ;;

    shell)
        echo "Starting interactive shell..."
        docker run -it --rm \
            -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            "$IMAGE_NAME" \
            /bin/bash
        ;;

    generate)
        CONFIG="${2:-RocketConfig}"
        echo "Generating Verilog for CONFIG=$CONFIG ..."
        docker run $DOCKER_TTY_FLAG --rm \
            -v "$PROJECT_ROOT:/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make verilog CONFIG=$CONFIG"
        ;;

    verilator)
        CONFIG="${2:-RocketConfig}"
        echo "Building Verilator simulator for CONFIG=$CONFIG ..."
        docker run $DOCKER_TTY_FLAG --rm \
            -v "$PROJECT_ROOT:/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make CONFIG=$CONFIG"
        ;;

    sim)
        CONFIG="${2:-RocketConfig}"
        BINARY="${3:?Usage: docker/docker-run.sh sim <CONFIG> <path-to-binary>}"
        echo "Running simulation CONFIG=$CONFIG BINARY=$BINARY ..."
        docker run $DOCKER_TTY_FLAG --rm \
            -v "$PROJECT_ROOT:/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make run-binary CONFIG=$CONFIG BINARY=/workspace/$BINARY"
        ;;

    bitstream)
        SUB_PROJECT="${2:-vc707}"
        CONFIG="${3:-RocketVC707Config}"
        MAC="${VIVADO_MAC:-0c:c4:7a:88:d2:7a}"
        LIC="${XILINXD_LICENSE_FILE:-$HOME/.Xilinx/Xilinx.lic}"
        echo "Building FPGA bitstream SUB_PROJECT=$SUB_PROJECT CONFIG=$CONFIG ..."
        docker run $DOCKER_TTY_FLAG --rm \
            --mac-address "$MAC" \
            -v "$PROJECT_ROOT:/workspace" \
            -v "$LIC:/root/.Xilinx/Xilinx.lic:ro" \
            -e XILINXD_LICENSE_FILE=/root/.Xilinx/Xilinx.lic \
            -w /workspace \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make -C fpga SUB_PROJECT=$SUB_PROJECT CONFIG=$CONFIG bitstream"
        ;;

    *)
        echo "Usage: docker/docker-run.sh <command> [args]"
        echo ""
        echo "Commands:"
        echo "  build                           Build the Docker image"
        echo "  shell                           Open interactive shell (default)"
        echo "  generate [CONFIG]               Generate Verilog (default: RocketConfig)"
        echo "  verilator [CONFIG]              Build Verilator simulator"
        echo "  sim <CONFIG> <BINARY>           Run simulation with a binary"
        echo "  bitstream [SUB_PROJECT] [CONFIG] Build FPGA bitstream (default: vc707 RocketVC707Config)"
        echo ""
        echo "Examples:"
        echo "  docker/docker-run.sh shell"
        echo "  docker/docker-run.sh generate RocketConfig"
        echo "  docker/docker-run.sh bitstream vc707 RocketVC707Config"
        echo ""
        echo "Vivado license env vars:"
        echo "  VIVADO_MAC            MAC address for node-locked license (default: 0c:c4:7a:88:d2:7a)"
        echo "  XILINXD_LICENSE_FILE  Path to Xilinx.lic on host (default: ~/.Xilinx/Xilinx.lic)"
        ;;
esac

#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
IMAGE_NAME="locnguyen96/chipyard-dev:latest"

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
        docker run -it --rm \
            -v "$PROJECT_ROOT:/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make verilog CONFIG=$CONFIG"
        ;;

    verilator)
        CONFIG="${2:-RocketConfig}"
        echo "Building Verilator simulator for CONFIG=$CONFIG ..."
        docker run -it --rm \
            -v "$PROJECT_ROOT:/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make CONFIG=$CONFIG"
        ;;

    sim)
        CONFIG="${2:-RocketConfig}"
        BINARY="${3:?Usage: docker/docker-run.sh sim <CONFIG> <path-to-binary>}"
        echo "Running simulation CONFIG=$CONFIG BINARY=$BINARY ..."
        docker run -it --rm \
            -v "$PROJECT_ROOT:/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make run-binary CONFIG=$CONFIG BINARY=$BINARY"
        ;;

    *)
        echo "Usage: docker/docker-run.sh <command> [args]"
        echo ""
        echo "Commands:"
        echo "  build                        Build the Docker image"
        echo "  shell                        Open interactive shell (default)"
        echo "  generate [CONFIG]            Generate Verilog (default: RocketConfig)"
        echo "  verilator [CONFIG]           Build Verilator simulator"
        echo "  sim <CONFIG> <BINARY>        Run simulation with a binary"
        echo ""
        echo "Examples:"
        echo "  docker/docker-run.sh shell"
        echo "  docker/docker-run.sh generate RocketConfig"
        echo "  docker/docker-run.sh verilator RocketConfig"
        ;;
esac

#!/bin/bash
set -e

IMAGE_NAME="locnguyen96/chipyard-dev:latest"

echo "=== Chipyard Docker Helper ==="
echo ""

case "${1:-shell}" in
    build)
        echo "Use docker/build.sh to build the image."
        echo "  cd docker && ./build.sh --platform linux/arm64"
        echo "  cd docker && ./build.sh --push"
        exit 0
        ;;

    shell)
        echo "Starting interactive shell..."
        docker run -it --rm \
            -v "$(pwd):/workspace" \
            -w /workspace \
            "$IMAGE_NAME" \
            /bin/bash
        ;;

    generate)
        # Generate Verilog for a given CONFIG (default: RocketConfig)
        CONFIG="${2:-RocketConfig}"
        echo "Generating Verilog for CONFIG=$CONFIG ..."
        docker run -it --rm \
            -v "$(pwd):/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make verilog CONFIG=$CONFIG"
        ;;

    verilator)
        # Build verilator simulator for a given CONFIG
        CONFIG="${2:-RocketConfig}"
        echo "Building Verilator simulator for CONFIG=$CONFIG ..."
        docker run -it --rm \
            -v "$(pwd):/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make CONFIG=$CONFIG"
        ;;

    sim)
        # Run simulation with a binary
        CONFIG="${2:-RocketConfig}"
        BINARY="${3:?Usage: ./docker-run.sh sim <CONFIG> <path-to-binary>}"
        echo "Running simulation CONFIG=$CONFIG BINARY=$BINARY ..."
        docker run -it --rm \
            -v "$(pwd):/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make run-binary CONFIG=$CONFIG BINARY=$BINARY"
        ;;

    *)
        echo "Usage: ./docker-run.sh <command> [args]"
        echo ""
        echo "Commands:"
        echo "  build                        Build the Docker image"
        echo "  shell                        Open interactive shell (default)"
        echo "  generate [CONFIG]            Generate Verilog (default: RocketConfig)"
        echo "  verilator [CONFIG]           Build Verilator simulator"
        echo "  sim <CONFIG> <BINARY>        Run simulation with a binary"
        echo ""
        echo "Examples:"
        echo "  ./docker-run.sh build"
        echo "  ./docker-run.sh shell"
        echo "  ./docker-run.sh generate RocketConfig"
        echo "  ./docker-run.sh verilator RocketConfig"
        ;;
esac

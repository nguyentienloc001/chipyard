#!/bin/bash
set -e

IMAGE_NAME="chipyard:latest"
PLATFORM="--platform linux/amd64"

echo "=== Chipyard Docker Helper ==="
echo ""

case "${1:-shell}" in
    build)
        echo "Building Docker image..."
        docker build $PLATFORM -t "$IMAGE_NAME" .
        echo "Done! Run: ./docker-run.sh shell"
        ;;

    shell)
        echo "Starting interactive shell..."
        docker run $PLATFORM -it --rm \
            -v "$(pwd):/workspace" \
            -w /workspace \
            "$IMAGE_NAME" \
            /bin/bash
        ;;

    generate)
        # Generate Verilog for a given CONFIG (default: RocketConfig)
        CONFIG="${2:-RocketConfig}"
        echo "Generating Verilog for CONFIG=$CONFIG ..."
        docker run $PLATFORM -it --rm \
            -v "$(pwd):/workspace" \
            -w /workspace/sims/verilator \
            "$IMAGE_NAME" \
            bash -c "source /opt/chipyard-env.sh && make verilog CONFIG=$CONFIG"
        ;;

    verilator)
        # Build verilator simulator for a given CONFIG
        CONFIG="${2:-RocketConfig}"
        echo "Building Verilator simulator for CONFIG=$CONFIG ..."
        docker run $PLATFORM -it --rm \
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
        docker run $PLATFORM -it --rm \
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

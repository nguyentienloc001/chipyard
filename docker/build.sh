#!/bin/bash
set -euo pipefail

#############################################################################
# Chipyard Docker Build & Push Script (Multi-Arch)
#
# Usage:
#   ./build.sh                          # Build for current arch (local)
#   ./build.sh --platform linux/arm64   # Build for specific arch
#   ./build.sh --push                   # Build amd64+arm64 and push to Hub
#   ./build.sh --tag v1.0.0 --push      # Build, tag, and push
#############################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# ── Configuration ─────────────────────────────────────────────────────────
DOCKER_REPO="${DOCKER_REPO:-locnguyen96/chipyard-dev}"
IMAGE_TAG="${IMAGE_TAG:-latest}"
PLATFORM=""
PUSH=false
NO_CACHE=false
BUILDER_NAME="chipyard-builder"

# ── Parse arguments ───────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --push)
            PUSH=true
            shift
            ;;
        --no-cache)
            NO_CACHE=true
            shift
            ;;
        --tag)
            IMAGE_TAG="$2"
            shift 2
            ;;
        --repo)
            DOCKER_REPO="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --platform <p>    Target platform (default: current arch)"
            echo "                    Examples: linux/arm64, linux/amd64"
            echo "                    For multi-arch push: linux/amd64,linux/arm64"
            echo "  --push            Build multi-arch (amd64+arm64) and push to Docker Hub"
            echo "  --no-cache        Build without Docker cache"
            echo "  --tag <tag>       Image tag (default: latest)"
            echo "  --repo <repo>     Docker Hub repo (default: locnguyen96/chipyard-dev)"
            echo "  --help            Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

FULL_IMAGE="${DOCKER_REPO}:${IMAGE_TAG}"

# Default platform logic
if [ "$PUSH" = true ] && [ -z "$PLATFORM" ]; then
    PLATFORM="linux/amd64,linux/arm64"
fi

# ── Ensure buildx builder exists ─────────────────────────────────────────
ensure_buildx() {
    if ! docker buildx inspect "$BUILDER_NAME" &>/dev/null; then
        echo "Creating buildx builder '${BUILDER_NAME}'..."
        docker buildx create --name "$BUILDER_NAME" --use --bootstrap
    else
        docker buildx use "$BUILDER_NAME"
    fi
}

# ── Build ─────────────────────────────────────────────────────────────────
BUILD_DATE=$(date -u +'%Y-%m-%dT%H:%M:%SZ')
VCS_REF=$(git -C "$PROJECT_ROOT" rev-parse --short HEAD 2>/dev/null || echo "unknown")

echo "═══════════════════════════════════════════════════════════════"
echo "  Chipyard Docker Build"
echo "═══════════════════════════════════════════════════════════════"
echo "  Image:     ${FULL_IMAGE}"
echo "  Platform:  ${PLATFORM:-current arch}"
echo "  Push:      ${PUSH}"
echo "  No-cache:  ${NO_CACHE}"
echo "  Date:      ${BUILD_DATE}"
echo "  Commit:    ${VCS_REF}"
echo "═══════════════════════════════════════════════════════════════"
echo ""

ensure_buildx

BUILD_ARGS=(
    --file "${SCRIPT_DIR}/Dockerfile"
    --build-arg "BUILD_DATE=${BUILD_DATE}"
    --build-arg "VCS_REF=${VCS_REF}"
    --tag "${FULL_IMAGE}"
)

if [ -n "$PLATFORM" ]; then
    BUILD_ARGS+=(--platform "${PLATFORM}")
fi

# Also tag as :latest if building a versioned tag
if [ "$IMAGE_TAG" != "latest" ]; then
    BUILD_ARGS+=(--tag "${DOCKER_REPO}:latest")
fi

if [ "$NO_CACHE" = true ]; then
    BUILD_ARGS+=(--no-cache)
fi

if [ "$PUSH" = true ]; then
    BUILD_ARGS+=(--push)
else
    BUILD_ARGS+=(--load)
fi

echo "Building..."
echo ""

docker buildx build "${BUILD_ARGS[@]}" "$PROJECT_ROOT"

echo ""
echo "═══════════════════════════════════════════════════════════════"
if [ "$PUSH" = true ]; then
    echo "  Build and push complete: ${FULL_IMAGE}"
    echo "  Platforms: ${PLATFORM}"
else
    echo "  Build complete: ${FULL_IMAGE}"
    echo "  Run: docker run -it --rm -v \$(pwd):/workspace ${FULL_IMAGE}"
fi
echo "═══════════════════════════════════════════════════════════════"

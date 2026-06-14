#!/bin/bash
# Build script for the MinecraftPE dedicated server.
# Runs cmake + make inside the build/ subdirectory next to this script.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Optional: pass -DBUILD_32BIT=ON as an argument to build a 32-bit binary.
CMAKE_EXTRA_ARGS=("$@")

JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

echo "=== MinecraftPE Dedicated Server Build ==="
echo "Source : $SCRIPT_DIR"
echo "Build  : $BUILD_DIR"
echo "Jobs   : $JOBS"
echo ""

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure (only re-runs if CMakeCache.txt is missing or args changed).
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "--- Configuring with CMake ---"
    cmake "$SCRIPT_DIR" "${CMAKE_EXTRA_ARGS[@]}"
    echo ""
fi

echo "--- Building ---"
make -j"$JOBS"

echo ""
echo "=== Build complete ==="
echo "Binary: $BUILD_DIR/mcpe_server"

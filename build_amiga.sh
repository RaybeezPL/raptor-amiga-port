#!/usr/bin/env bash
# build_amiga.sh
# Raptor for AmigaOS 3.x on 68060 with FPU

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build.amiga"

export PATH="/opt/amiga/bin:$PATH"

cd "$PROJECT_DIR"

echo "=== Cleaning Amiga build directory ==="

rm -rf -- "$BUILD_DIR"
rm -f -- "$PROJECT_DIR/raptor"

echo "=== Building Raptor for 68060 with FPU ==="

make -f Makefile.amiga \
    BUILD_DIR=build.amiga \
    VERBOSE=1

echo "=== Build complete ==="

file raptor
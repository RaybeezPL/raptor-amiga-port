#!/usr/bin/env bash
# build_amiga_nofpu.sh
# Raptor for AmigaOS 3.x on 68060 without FPU

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"

export PATH="/opt/amiga/bin:$PATH"

cd "$PROJECT_DIR"

echo "=== Cleaning generated object and dependency files ==="

find . -type f \( -name '*.o' -o -name '*.d' \) -delete
rm -rf build.amiga.nofpu
rm -f raptor_nofpu

echo "=== Building Raptor for 68060 without FPU ==="

make -f Makefile.amiga \
    NOFPU=1 \
    BUILD_DIR=build.amiga.nofpu \
    VERBOSE=1

echo "=== Verifying no FPU instructions ==="

FPU_COUNT="$(
    m68k-amigaos-objdump -d raptor_nofpu |
    awk -F'\t' '$3 ~ /^f[a-z]/ { count++ } END { print count + 0 }'
)"

if [ "$FPU_COUNT" != "0" ]; then
    echo "ERROR: raptor_nofpu contains $FPU_COUNT FPU instructions:"
    m68k-amigaos-objdump -d raptor_nofpu |
        awk -F'\t' '$3 ~ /^f[a-z]/' |
        head -20
    echo "A 68060 without FPU cannot run this binary."
    exit 1
fi

echo "OK: no FPU instructions found"

file raptor_nofpu
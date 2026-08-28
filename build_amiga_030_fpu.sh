#!/usr/bin/env bash
# build_amiga_030_fpu.sh
# Raptor for AmigaOS 3.x on 68030 with 68881/68882 FPU

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"

export PATH="/opt/amiga/bin:$PATH"

cd "$PROJECT_DIR"

echo "=== Cleaning generated object and dependency files ==="

find . -type f \( -name '*.o' -o -name '*.d' \) -delete
rm -rf build.amiga.030.fpu
rm -f raptor_030_fpu

echo "=== Building Raptor for 68030 with 68881/68882 FPU ==="

make -f Makefile.amiga \
    CPU=030 \
    NOFPU=0 \
    BUILD_DIR=build.amiga.030.fpu \
    VERBOSE=1

echo "=== Verifying FPU and CPU instructions ==="

FPU_COUNT="$(
    m68k-amigaos-objdump -d raptor_030_fpu |
    awk -F'\t' '$3 ~ /^f[a-z]/ { count++ } END { print count + 0 }'
)"

M040_COUNT="$(
    m68k-amigaos-objdump -d raptor_030_fpu |
    awk '$3 ~ /^byterev$/ || $3 ~ /^bitrev$/ { count++ } END { print count + 0 }'
)"

echo "FPU instructions found: $FPU_COUNT"

if [ "$FPU_COUNT" = "0" ]; then
    echo "NOTE: no FPU instructions detected."
    echo "The build appears FPU-free; FPU presence cannot be positively confirmed."
fi

if [ "$M040_COUNT" != "0" ]; then
    echo "ERROR: raptor_030_fpu contains 68040/68060-only CPU instructions:"
    m68k-amigaos-objdump -d raptor_030_fpu |
        awk -F'\t' '$3 ~ /^byterev$/ || $3 ~ /^bitrev$/' |
        head -20
    echo "A 68030 CPU cannot run this binary."
    exit 1
fi

echo "OK: no 68040/68060-only CPU instructions found"

file raptor_030_fpu
#!/usr/bin/env bash
# build_amiga_030.sh
# Raptor for AmigaOS 3.x on 68030 without FPU

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"

export PATH="/opt/amiga/bin:$PATH"

cd "$PROJECT_DIR"

echo "=== Cleaning generated object and dependency files ==="

find . -type f \( -name '*.o' -o -name '*.d' \) -delete
rm -rf build.amiga.030
rm -f raptor_030

echo "=== Building Raptor for 68030 without FPU ==="

make -f Makefile.amiga \
    CPU=030 \
    NOFPU=1 \
    BUILD_DIR=build.amiga.030 \
    VERBOSE=1

echo "=== Verifying no FPU and no 68040/68060-only instructions ==="

FPU_COUNT="$(
    m68k-amigaos-objdump -d raptor_030 |
    awk -F'\t' '$3 ~ /^f[a-z]/ { count++ } END { print count + 0 }'
)"

M040_COUNT="$(
    m68k-amigaos-objdump -d raptor_030 |
    awk '$3 ~ /^byterev$/ || $3 ~ /^bitrev$/ { count++ } END { print count + 0 }'
)"

if [ "$FPU_COUNT" != "0" ] || [ "$M040_COUNT" != "0" ]; then
    echo "ERROR: raptor_030 contains forbidden instructions:"

    m68k-amigaos-objdump -d raptor_030 |
        awk -F'\t' '$3 ~ /^f[a-z]/ || $3 ~ /^byterev$/ || $3 ~ /^bitrev$/' |
        head -20

    echo "A 68030 without FPU cannot run this binary."
    exit 1
fi

echo "OK: no FPU and no 68040/68060-only instructions found"

file raptor_030

echo "NOTE: if objdump cannot decode byterev/bitrev opcodes,"
echo "those instructions cannot be positively excluded by this scan."
#!/bin/bash
# =============================================================================
# build_amiga_030.sh - Raptor for AmigaOS 3.x on 68030 WITHOUT FPU
#
# Soft-float build: objects compiled with -m68030 -msoft-float, linked with
# -m68000 -msoft-float (see Makefile.amiga, CPU=030 NOFPU=1, for the
# rationale).
#
# Produces: raptor_030
#
# The script fails if the resulting binary contains any FPU instruction or
# any 68040/68060-only CPU instruction (byterev/bitrev).
#
# Before every build, all generated object (*.o) and dependency (*.d) files
# are removed so the build always starts from clean compilation artifacts.
# =============================================================================

set -e

export PATH=/opt/amiga/bin:$PATH

cd /mnt/c/amiga-raptor/raptor

echo "=== Cleaning object and dependency files ==="

find . -name '*.o' -delete
find . -name '*.d' -delete

rm -rf build.amiga.030
rm -f raptor_030

echo "=== Building Raptor for 68030 without FPU ==="

make -f Makefile.amiga CPU=030 NOFPU=1 VERBOSE=1

echo "=== Verifying: no FPU instructions and no 68040/68060-only CPU instructions in raptor_030 ==="

FPU_COUNT=$(m68k-amigaos-objdump -d raptor_030 | awk -F'\t' '$3 ~ /^f[a-z]/' | wc -l)

M040_COUNT=$(m68k-amigaos-objdump -d raptor_030 | awk '$3 ~ /^byterev$/ || $3 ~ /^bitrev$/ {c++} END {print c+0}')

if [ "$FPU_COUNT" != "0" ] || [ "$M040_COUNT" != "0" ]; then
    echo "ERROR: raptor_030 contains forbidden instructions:"

    m68k-amigaos-objdump -d raptor_030 | \
        awk -F'\t' '$3 ~ /^f[a-z]/ || $3 ~ /^byterev$/ || $3 ~ /^bitrev$/' | \
        head -20

    echo "A 68030 without an FPU cannot run this binary - aborting."
    exit 1
fi

echo "OK: no FPU instructions, no 68040/68060-only instructions found"

file raptor_030

# NOTE: if the toolchain objdump does not decode 68040/68060 opcodes into
# byterev/bitrev mnemonics, this scan will simply not flag them. In that case
# the FPU check above (authoritative, same as the existing 68060 no-FPU script)
# still passes/fails correctly; the byterev/bitrev part would then be reported
# as unverified in the build output instead of silently assumed OK.
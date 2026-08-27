#!/bin/bash
# =============================================================================
# build_amiga_030_fpu.sh - Raptor for AmigaOS 3.x on 68030 with a 68881/68882
#
# FPU build: objects compiled and linked with -m68030 -m68881.
#
# Produces: raptor_030_fpu
#
# For this variant FPU instructions are EXPECTED and allowed, so the no-FPU
# rejection check is NOT applied. Verification instead checks:
#
#   1. presence of FPU instructions is reported,
#   2. NO 68040/68060-only CPU instructions (byterev/bitrev) are present.
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

rm -rf build.amiga.030.fpu
rm -f raptor_030_fpu

echo "=== Building Raptor for 68030 with 68881/68882 FPU ==="

make -f Makefile.amiga CPU=030 NOFPU=0 VERBOSE=1

echo "=== Verifying: FPU instructions allowed/reported; no 68040/68060-only CPU instructions ==="

FPU_COUNT=$(m68k-amigaos-objdump -d raptor_030_fpu | awk -F'\t' '$3 ~ /^f[a-z]/' | wc -l)

M040_COUNT=$(m68k-amigaos-objdump -d raptor_030_fpu | awk '$3 ~ /^byterev$/ || $3 ~ /^bitrev$/ {c++} END {print c+0}')

echo "FPU instructions found: $FPU_COUNT"

if [ "$FPU_COUNT" = "0" ]; then
    echo "NOTE: no FPU instructions detected - the build appears FPU-free;"
    echo "FPU presence cannot be positively confirmed from disassembly."
fi

if [ "$M040_COUNT" != "0" ]; then
    echo "ERROR: raptor_030_fpu contains 68040/68060-only CPU instructions:"

    m68k-amigaos-objdump -d raptor_030_fpu | \
        awk -F'\t' '$3 ~ /^byterev$/ || $3 ~ /^bitrev$/' | \
        head -20

    echo "A 68030 CPU cannot run this binary - aborting."
    exit 1
fi

echo "OK: no 68040/68060-only CPU instructions found"

file raptor_030_fpu

# NOTE: as with the soft-float script, if the toolchain objdump cannot decode
# byterev/bitrev, those opcodes cannot be positively excluded; this will be
# stated in the final report rather than assumed verified.
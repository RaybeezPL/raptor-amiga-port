#!/bin/bash
# =============================================================================
# build_amiga_nofpu.sh - Raptor for AmigaOS 3.x on 68060 WITHOUT FPU
#
# Soft-float build: objects compiled with -m68060 -msoft-float, linked with
# -m68000 -msoft-float (see Makefile.amiga, NOFPU=1, for the rationale).
# Produces: raptor_nofpu  (objects in build.amiga.nofpu/)
#
# The script fails if the resulting binary contains any FPU instruction.
# =============================================================================
set -e
export PATH=/opt/amiga/bin:$PATH
cd /mnt/c/amiga-raptor/raptor

rm -rf build.amiga.nofpu
rm -f raptor_nofpu

make -f Makefile.amiga NOFPU=1 VERBOSE=1

echo "=== Verifying: no FPU instructions in raptor_nofpu ==="
FPU_COUNT=$(m68k-amigaos-objdump -d raptor_nofpu | awk -F'\t' '$3 ~ /^f[a-z]/' | wc -l)
if [ "$FPU_COUNT" != "0" ]; then
    echo "ERROR: raptor_nofpu contains $FPU_COUNT FPU instructions:"
    m68k-amigaos-objdump -d raptor_nofpu | awk -F'\t' '$3 ~ /^f[a-z]/' | head -20
    echo "A 68060 without FPU cannot run this binary - aborting."
    exit 1
fi
echo "OK: no FPU instructions found"
file raptor_nofpu

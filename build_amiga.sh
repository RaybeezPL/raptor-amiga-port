#!/bin/bash
# =============================================================================
# build_amiga.sh - Raptor for AmigaOS 3.x on 68060 with FPU
#
# Produces: raptor
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

rm -rf build.amiga
rm -f raptor

echo "=== Building Raptor for 68060 with FPU ==="

make -f Makefile.amiga VERBOSE=1

echo "=== Build complete ==="

file raptor
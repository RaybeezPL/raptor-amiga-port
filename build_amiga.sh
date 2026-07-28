#!/bin/bash
set -e
export PATH=/opt/amiga/bin:$PATH
cd /mnt/c/amiga-raptor/raptor
find . -name '*.o' -delete
find . -name '*.d' -delete
rm -f raptor
make -f Makefile.amiga VERBOSE=1
file raptor

#!/bin/bash
set -e
export PATH=/opt/amiga/bin:$PATH
cd /mnt/c/amiga-raptor/raptor
make -f Makefile.amiga clean
make -f Makefile.amiga VERBOSE=1

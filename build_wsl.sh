#!/bin/bash
export PATH=/opt/amiga/bin:$PATH
cd /mnt/c/amiga-raptor/raptor
make -f Makefile.amiga 2>&1

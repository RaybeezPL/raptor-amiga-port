#!/usr/bin/env bash
set -Eeuo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
binary=$(mktemp /tmp/raptor-amiga-cfg-test-XXXXXX)
trap 'rm -f -- "$binary"' EXIT

g++ -std=c++11 -Wall -Wextra -Werror -D__AMIGA__ -DAMIGA_CFG_TEST \
    -I"$root/src" "$root/tests/amiga_cfg_test.cpp" \
    "$root/src/amiga/amiga_cfg.cpp" -o "$binary"
"$binary"
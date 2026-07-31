#!/usr/bin/env bash
set -euo pipefail

# Główny katalog projektu (tutaj, gdzie jest CMakeLists.txt pod PC)
PROJECT_DIR="/mnt/c/amiga-raptor/raptor"

cd "$PROJECT_DIR"

BUILD_DIR="build"

# Czysta konfiguracja
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Konfiguracja CMake pod Windows (MinGW) z WSL
cmake .. -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++

# Budowa
cmake --build . --config Release -j"$(nproc)"

echo "Build finished. Windows executable should be in: $PWD"

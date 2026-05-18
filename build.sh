#!/usr/bin/env bash
# build.sh — Linux MinGW cross-compile to Windows x64
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build"

mkdir -p "$BUILD_DIR"

echo "Configuring CMake (MinGW x86_64-w64-mingw32)..."
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
    -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build "$BUILD_DIR" --parallel

ASI="$BUILD_DIR/mempatch.asi"
if [[ -f "$ASI" ]]; then
    SIZE=$(stat -c %s "$ASI" 2>/dev/null || stat -f %z "$ASI")
    SIZE_KB=$(( SIZE / 1024 ))
    echo "Built: $ASI (${SIZE_KB} KB)"
else
    echo "Expected mempatch.asi not found" >&2
    exit 1
fi

#!/bin/bash
# Build the C++ venim package manager.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$ROOT/src-cpp" -B "$ROOT/build-cpp" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$ROOT/build-cpp" -j"$(nproc)"
echo "built $ROOT/build-cpp/venim"
"$ROOT/build-cpp/venim" --version

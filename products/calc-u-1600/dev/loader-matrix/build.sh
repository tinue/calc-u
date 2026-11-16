#!/bin/sh
# Builds the loader-matrix harness into headless/loader-matrix/.
# Usage: dev/loader-matrix/build.sh
#
# The harness drives the machine interface (C1600-1), so it links the core
# and the library as the root CMake project builds them (calcu_core,
# calcu_library), in the tree CALCU_BUILD (default build/tools at the
# repository root, configured here if it isn't yet).
set -e
cd "$(dirname "$0")/../.."
ROOT=../..
BUILD=${CALCU_BUILD:-$ROOT/build/tools}
[ -f "$BUILD/CMakeCache.txt" ] || cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCALCU_FRONTEND_QT=OFF -DCALCU_PRODUCT_59=OFF -DCALCU_TESTS=OFF
cmake --build "$BUILD" --target calcu1600_harness
mkdir -p headless/loader-matrix
clang++ -std=c++20 -Wall -Wextra -O2 -I core -I . -I "$ROOT/library" \
  dev/loader-matrix/pc1500_loader_matrix.cpp \
  "$BUILD/products/calc-u-1600/libcalcu1600_harness.a" "$BUILD/products/calc-u-1600/libcalcu_shared.a" \
  "$BUILD/products/calc-u-1600/libcalcu_core.a" "$BUILD/library/libcalcu_library.a" \
  core/Basic/vendor/sharpdx/libsharpdx.a \
  -o headless/loader-matrix/pc1500_loader_matrix

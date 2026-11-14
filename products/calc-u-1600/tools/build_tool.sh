#!/bin/sh
# Builds headless tools of the root CMake project (ADR 0003) in the
# repository's build/tools tree (Release, no Qt) and copies them into
# headless/. The sources come from CMake (cmake/CalcU.cmake and the
# product's CMakeLists.txt), so no script lists core/ files.
#
#   tools/build_tool.sh pc1500_cli pc1600_cli ...
set -e
cd "$(dirname "$0")/.."
REPO="$(cd ../.. && pwd)"
TREE="$REPO/build/tools"
if [ ! -f "$TREE/CMakeCache.txt" ]; then
  cmake -S "$REPO" -B "$TREE" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCALCU_FRONTEND_QT=OFF >/dev/null
fi
cmake --build "$TREE" --target "$@"
mkdir -p headless
for target in "$@"; do
  cp "$TREE/products/calc-u-1600/$target" headless/
done

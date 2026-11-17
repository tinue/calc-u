#!/bin/sh
# Builds and runs Calc-U-1600's Frequent tests from the repository's root
# CMake project (ADR 0003) without the Qt apps, in the repository's
# build/tests tree (Debug: core and tests at -O1).
#
#   tools/run_tests.sh            every Frequent CTest entry of the product
#                                 (CoreTests, the session runners, DAP, help,
#                                 the preset and source checks)
#   tools/run_tests.sh <text>     the TESTs whose name contains <text>, in
#                                 every runner program of the product
#
# The tests run from the product directory (they load roms/ via relative
# paths). Since D P-10 the tests are several programs, not CoreTests alone
# (review E F-8).
set -e
cd "$(dirname "$0")/.."
REPO="$(cd ../.. && pwd)"
TREE="$REPO/build/tests"
OUT="$TREE/products/calc-u-1600"
RUNNERS="CoreTests CalcU1600SessionTests CalcU1600AppSessionTests CalcU1600DapTests CalcU1600HelpTests"
if [ ! -f "$TREE/CMakeCache.txt" ]; then
  cmake -S "$REPO" -B "$TREE" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCALCU_FRONTEND_QT=OFF >/dev/null
fi
if [ $# -eq 0 ]; then
  # calcu: the preset checks run on the harness.
  cmake --build "$TREE" --target $RUNNERS calcu
  exec ctest --test-dir "$OUT" --output-on-failure
fi
cmake --build "$TREE" --target $RUNNERS
status=0
for runner in $RUNNERS; do
  echo "── $runner"
  "$OUT/$runner" "$@" || status=1
done
exit $status

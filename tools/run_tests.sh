#!/bin/sh
# Builds and runs a product's Frequent tests from the repository's root
# CMake project without the Qt apps, in the repository's build/tests tree
# (configured the first time as Debug: the cores and the test programs at
# -O1).
#
#   tools/run_tests.sh <product>           every Frequent CTest entry of the
#                                          product (its runners, the help,
#                                          the preset and source checks)
#   tools/run_tests.sh <product> <text>    the TESTs whose name contains
#                                          <text>, in every runner program
#                                          of the product
#   tools/run_tests.sh <product> --group longer [<text>]
#                                          the same in the runners' Longer
#                                          TESTs (also checkpoint, all)
#
# <product> is calc-u-59 or calc-u-1600 (or 59, 1600). The runners start in
# the product directory, as CTest starts them: they load roms/ and their
# fixtures by relative paths.
set -e
case "${1:-}" in
  59|calc-u-59) PRODUCT=calc-u-59 RUNNERS="CalcU59Tests" ;;
  1600|calc-u-1600)
    PRODUCT=calc-u-1600
    RUNNERS="CalcU1600CoreTests CalcU1600SessionTests CalcU1600AppSessionTests CalcU1600DapTests CalcU1600HelpTests" ;;
  *) echo "usage: tools/run_tests.sh calc-u-59|calc-u-1600 [--group <group>] [name filter]" >&2; exit 2 ;;
esac
shift
REPO="$(cd "$(dirname "$0")/.." && pwd)"
TREE="$REPO/build/tests"
OUT="$TREE/products/$PRODUCT"
cd "$REPO/products/$PRODUCT"
if [ ! -f "$TREE/CMakeCache.txt" ]; then
  cmake -S "$REPO" -B "$TREE" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCALCU_FRONTEND_QT=OFF >/dev/null
fi
if [ $# -eq 0 ]; then
  # calcu: the preset checks and the harness runs are on it.
  cmake --build "$TREE" --target $RUNNERS calcu
  exec ctest --test-dir "$OUT" -L frequent --output-on-failure
fi
cmake --build "$TREE" --target $RUNNERS
status=0
for runner in $RUNNERS; do
  echo "── $runner"
  "$OUT/$runner" "$@" || status=1
done
exit $status

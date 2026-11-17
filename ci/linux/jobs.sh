#!/bin/bash
# Runs inside the container (ci/linux/run.sh starts it): the Linux jobs of
# .github/workflows/calc-u-59.yml, calc-u-1600.yml and library.yml (which
# both call) on the copy in /work.
# Each job is the workflow job's configure, build and test commands; the
# build trees get one name per job (CI has one machine per job).
#
#   library     library.yml's `library` job (both workflows' `library`)
#   core-59     calc-u-59.yml `core-tests`
#   core-1600   calc-u-1600.yml `core-tests`
#   qt-59       calc-u-59.yml `linux` up to the build (no AppImage)
#   qt-1600     calc-u-1600.yml `linux` up to the build (no AppImage)
#   web-59      calc-u-59.yml `web` up to the tests (web image only)
#   web-1600    calc-u-1600.yml `web` up to the tests (web image only)
#   help-59     calc-u-59.yml `help`: the help asset (no upload)
#   help-1600   calc-u-1600.yml `help`: the help asset (no upload)
set -uo pipefail
cd /work

sharpdx() {  # the 1600 jobs' "Fetch sharpdx", once per volume
  [ -f products/calc-u-1600/core/Basic/vendor/sharpdx/libsharpdx-linux.a ] \
    || products/calc-u-1600/tools/fetch_sharpdx.sh
}

emsdk() {  # emsdk <version>: the web job's setup-emsdk, without ccache
  unset CMAKE_C_COMPILER_LAUNCHER CMAKE_CXX_COMPILER_LAUNCHER
  # shellcheck disable=SC1090
  source "/opt/emsdk-$1/emsdk_env.sh" >/dev/null 2>&1
}

job() {
  case "$1" in
    library)
      cmake -S . -B build/library -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCALCU_PRODUCT_59=OFF -DCALCU_PRODUCT_1600=OFF -DCALCU_FRONTEND_QT=OFF &&
      cmake --build build/library &&
      ctest --test-dir build/library --output-on-failure ;;
    core-59)
      cd products/calc-u-59 &&
      cmake -S ../.. -B ../../build/tests-59 -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCALCU_PRODUCT_1600=OFF -DCALCU_FRONTEND_QT=OFF &&
      cmake --build ../../build/tests-59 &&
      ctest --test-dir ../../build/tests-59 --output-on-failure ;;
    core-1600)
      sharpdx && cd products/calc-u-1600 &&
      cmake -S ../.. -B ../../build/tests-1600 -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCALCU_PRODUCT_59=OFF -DCALCU_FRONTEND_QT=OFF &&
      cmake --build ../../build/tests-1600 &&
      ctest --test-dir ../../build/tests-1600 --output-on-failure ;;
    qt-59)
      cd products/calc-u-59 &&
      cmake -S ../.. -B ../../build/release-59 -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCALCU_PRODUCT_1600=OFF -DCALCU_TESTS=OFF &&
      cmake --build ../../build/release-59 --target CalcU59Qt calcu ;;
    qt-1600)
      sharpdx && cd products/calc-u-1600 &&
      cmake -S ../.. -B ../../build/release-1600 -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCALCU_PRODUCT_59=OFF -DCALCU_TESTS=OFF &&
      cmake --build ../../build/release-1600 --target CalcU1600Qt calcu ;;
    web-59)
      emsdk 6.0.12 && cd products/calc-u-59 &&
      frontends/web/build.sh &&
      frontends/web/run-tests.sh --reporter=line &&
      (cd /work/library/web && npm ci --no-audit --no-fund && tests/run-tests.sh --reporter=line) ;;
    web-1600)
      emsdk 6.0.13 && cd products/calc-u-1600 &&
      tools/build_wasm_check.sh &&
      frontends/web/build.sh &&
      (cd frontends/web && npm ci --no-audit --no-fund) &&
      frontends/web/run-tests.sh --reporter=line &&
      (cd /work/library/web && npm ci --no-audit --no-fund && tests/run-tests.sh --reporter=line) ;;
    help-59)
      python3 tools/help build calc-u-59 --out build/help --quiet-warnings ;;
    help-1600)
      python3 tools/help build calc-u-1600 --out build/help --quiet-warnings ;;
    *)
      echo "unknown job: $1" >&2; return 2 ;;
  esac
}

failed=()
summary=()
for j in "$@"; do
  echo "::: $j"
  start=$SECONDS
  ( job "$j" )
  status=$?
  took=$((SECONDS - start))
  if [ $status -eq 0 ]; then summary+=("pass  ${took}s  $j")
  else summary+=("FAIL  ${took}s  $j"); failed+=("$j"); fi
done

echo
echo "::: summary"
printf '  %s\n' "${summary[@]}"
ccache -s 2>/dev/null | grep -E "Hits|Misses" | sed 's/^/  ccache /' | head -2
[ ${#failed[@]} -eq 0 ]

#!/bin/bash
# Runs inside the container (ci/linux/run.sh starts it). Only what can fail
# on GitHub's Linux runners and not on the Mac: GCC 13 and libstdc++ (a
# missing include, a stricter overload, an internal compiler error, the
# order of argument evaluation at run time), Ubuntu's Qt and its moc, the
# pinned Emscripten and Playwright in Linux browsers. What gives the same
# result on every host (the help, the source checks) is the Mac's Frequent
# run.
#
#   gcc        one tree with both products, the Qt frontends and the tests:
#              the build of the workflows' library, core-tests and linux
#              jobs, then the CTest entries those jobs run (not the Qt
#              window tests, which no Linux job runs); among them
#              CalcUQtInstallLayout, the `cmake --install` layout the linux
#              jobs make their AppImages of (no AppImage is made here)
#   web-59     calc-u-59.yml `web` up to its tests (web image only; no zip)
#   web-1600   calc-u-1600.yml `web` up to the release dist/ and its check,
#              and the library's Web shell tests (web image only; no zip)
set -uo pipefail
cd /work

sharpdx() {  # the 1600 jobs' "Fetch sharpdx", once per volume
  [ -f products/calc-u-1600/core/Basic/vendor/sharpdx/libsharpdx-linux.a ] \
    || products/calc-u-1600/tools/fetch_sharpdx.sh
}

emsdk() {  # the web jobs' setup-emsdk (the image's one release), without ccache
  unset CMAKE_C_COMPILER_LAUNCHER CMAKE_CXX_COMPILER_LAUNCHER
  # shellcheck disable=SC1091
  source /opt/emsdk/emsdk_env.sh >/dev/null 2>&1
}

job() {
  case "$1" in
    gcc)
      sharpdx &&
      cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo &&
      cmake --build build/linux &&
      ctest --test-dir build/linux --output-on-failure -j8 \
        -E '^(calcu_qt6_tests|calcu_qt6_scenarios|CalcU59QtTests|CalcU1600QtTests)' ;;
    web-59)
      emsdk && cd products/calc-u-59 &&
      frontends/web/build.sh &&
      (cd frontends/web && npm ci --no-audit --no-fund) &&
      frontends/web/run-tests.sh --reporter=line ;;
    web-1600)
      emsdk && cd products/calc-u-1600 &&
      tools/build_wasm_check.sh &&
      frontends/web/build.sh &&
      (cd frontends/web && npm ci --no-audit --no-fund) &&
      frontends/web/run-tests.sh --reporter=line &&
      (cd /work/library/web && npm ci --no-audit --no-fund && tests/run-tests.sh --reporter=line) &&
      frontends/web/build.sh --release && tests/web_no_test_aids.sh frontends/web/dist ;;
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

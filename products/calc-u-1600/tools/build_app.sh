#!/usr/bin/env bash
# Configures (first time only) and builds the Qt6 app from the root CMake
# project (ADR 0003) into the repository's build/app-1600 tree; prints
# nothing else, and the app lands in the directory `app_dir` names.
# Shared by build_and_run.sh and tools/make_screenshots.sh.
#
#   tools/build_app.sh            # build
#   tools/build_app.sh app_dir    # print the directory holding the built app
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TREE="${REPO}/build/app-1600"

if [[ "${1:-}" == "app_dir" ]]; then
  echo "${TREE}/products/calc-u-1600/frontends/qt6"
  exit 0
fi

if [[ ! -f "${TREE}/build.ninja" ]]; then
  cmake -S "${REPO}" -B "${TREE}" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCALCU_PRODUCT_59=OFF
fi
cmake --build "${TREE}"

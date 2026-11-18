#!/usr/bin/env bash
# Regenerates the help's pictures (help/images/) by playing
# the scenarios in docs/developer/screenshots/guide/ (one per help chapter)
# against the Qt app through harness attach -- see
# docs/developer/screenshots/README.md. Builds the app first if needed.
#
#   tools/make_screenshots.sh                  # every docs/developer/screenshots/guide/*.shots.yaml
#   tools/make_screenshots.sh 04-media          # just these (names without .shots.yaml)
#
# Extra --shots-* options can follow the names after `--`, e.g.
#   tools/make_screenshots.sh 08-settings -- --shots-only 09-settings
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$("${ROOT}/../../tools/run_qt.sh" 1600 --app-dir)"

"${ROOT}/../../tools/run_qt.sh" 1600 --build

if [[ "$(uname -s)" == "Darwin" ]]; then
  APP="${BUILD_DIR}/Calc-U-1600.app/Contents/MacOS/Calc-U-1600"
else
  APP="${BUILD_DIR}/Calc-U-1600"
fi

names=()
extra=()
while [[ $# -gt 0 ]]; do
  if [[ "$1" == "--" ]]; then shift; extra=("$@"); break; fi
  names+=("$1"); shift
done

scenarios=()
if [[ ${#names[@]} -eq 0 ]]; then
  scenarios=("${ROOT}"/docs/developer/screenshots/guide/*.shots.yaml)
else
  for n in "${names[@]}"; do scenarios+=("${ROOT}/docs/developer/screenshots/guide/${n}.shots.yaml"); done
fi

status=0
for s in "${scenarios[@]}"; do
  "${APP}" --shots "${s}" ${extra[@]+"${extra[@]}"} || status=1
done
exit ${status}

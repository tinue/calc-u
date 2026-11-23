#!/usr/bin/env bash
# One version of each pinned toolchain in the repository:
#   Playwright  the three package-lock.json files (@playwright/test,
#               playwright, playwright-core), the Playwright container of
#               both product workflows' `web` jobs and the local CI's web
#               image; the container's browsers belong to one Playwright
#               release, with another version locked the tests would look
#               for browsers the container does not have.
#   Emscripten  setup-emsdk of both `web` jobs, the local CI's
#               EMSDK_VERSION, and the release libsharpdx-wasm.a was built
#               with (its README and docs/Building.md): Rust's std for
#               wasm32-unknown-emscripten uses native wasm exceptions, so
#               the linking Emscripten matches the building one.
#   Node        the major version of setup-node in both `web` jobs and of
#               the local CI's NODE_VERSION.
# Reads files only (no network). CTest LibraryToolchainVersions (Frequent;
# not on Windows).
#
#   check_toolchain_versions.sh [<repository root>]
set -euo pipefail

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
WORKFLOWS=(.github/workflows/calc-u-59.yml .github/workflows/calc-u-1600.yml)
CONTAINERFILE=ci/linux/Containerfile
status=0

# same <tool> "<version>  <where>"...: every version the same, else the list.
same() {
  local tool=$1; shift
  local first=${1%%  *} f ok=1
  for f in "$@"; do
    [ "${f%%  *}" = "$first" ] || ok=0
  done
  if [ $ok = 1 ]; then
    echo "$tool $first everywhere ($# places)"
  else
    echo "$tool versions differ:"
    printf '  %s\n' "$@"
    status=1
  fi
}

# value <file> <sed script>: the first value the script prints, "missing"
# when there is none.
value() {
  local v
  v=$(sed -n "$2" "$1" | head -n1)
  echo "${v:-missing}"
}

# ---- Playwright
found=()
for dir in products/calc-u-59/frontends/web library/web products/calc-u-1600/frontends/web; do
  lock="$dir/package-lock.json"
  for pkg in @playwright/test playwright playwright-core; do
    # The first "version" after the package's "node_modules/<pkg>" entry.
    v=$(awk -v key="\"node_modules/$pkg\": {" '
      index($0, key) { inside = 1; next }
      inside && /"version":/ { gsub(/.*"version": *"|".*/, ""); print; exit }' "$lock")
    found+=("${v:-missing}  $lock $pkg")
  done
done
for file in "${WORKFLOWS[@]}" "$CONTAINERFILE"; do
  tags=$(grep -o 'mcr\.microsoft\.com/playwright:v[^ "]*' "$file" || true)
  [ -n "$tags" ] || { found+=("missing  $file (no mcr.microsoft.com/playwright image)"); continue; }
  while IFS= read -r tag; do
    v=${tag#*:v}
    found+=("${v%-noble}  $file $tag")
  done <<< "$tags"
done
same Playwright "${found[@]}"

# ---- Emscripten
found=()
for file in "${WORKFLOWS[@]}"; do
  # The `version:` of the step that uses setup-emsdk.
  v=$(awk '/uses: mymindstorm\/setup-emsdk@/ { inside = 1; next }
           inside && /^ *- / { exit }
           inside && /^ *version:/ { sub(/.*version: */, ""); sub(/ *#.*/, ""); print; exit }' "$file")
  found+=("${v:-missing}  $file setup-emsdk")
done
found+=("$(value "$CONTAINERFILE" 's/^ARG EMSDK_VERSION=\([0-9.]*\).*/\1/p')  $CONTAINERFILE EMSDK_VERSION")
readme=products/calc-u-1600/core/Basic/vendor/sharpdx/README.md
found+=("$(value "$readme" 's/.*the one it was built with (\([0-9.]*\)).*/\1/p')  $readme libsharpdx-wasm.a")
found+=("$(value docs/Building.md 's/.*built with Emscripten \([0-9.]*[0-9]\).*/\1/p')  docs/Building.md libsharpdx-wasm.a")
same Emscripten "${found[@]}"

# ---- Node (major version)
found=()
for file in "${WORKFLOWS[@]}"; do
  found+=("$(value "$file" 's/^ *node-version: *\([0-9]*\).*/\1/p')  $file setup-node")
done
found+=("$(value "$CONTAINERFILE" 's/^ARG NODE_VERSION=\([0-9]*\).*/\1/p')  $CONTAINERFILE NODE_VERSION")
same "Node major" "${found[@]}"

exit $status

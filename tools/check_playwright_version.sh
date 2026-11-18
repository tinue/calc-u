#!/usr/bin/env bash
# One Playwright version in the repository: the three package-lock.json files
# (@playwright/test, playwright, playwright-core), the Playwright container
# of both product workflows' `web` jobs and the local CI's web image. The
# container's browsers belong to one Playwright release; with another version
# locked, the tests would look for browsers the container does not have.
# Reads files only (no network). CTest LibraryPlaywrightVersion (Frequent;
# not on Windows).
#
#   check_playwright_version.sh [<repository root>]
set -euo pipefail

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
LOCKS=(products/calc-u-59/frontends/web library/web products/calc-u-1600/frontends/web)
IMAGES=(.github/workflows/calc-u-59.yml .github/workflows/calc-u-1600.yml ci/linux/Containerfile)

found=()   # "<version>  <where>"

for dir in "${LOCKS[@]}"; do
  lock="$dir/package-lock.json"
  for pkg in @playwright/test playwright playwright-core; do
    # The first "version" after the package's "node_modules/<pkg>" entry.
    v=$(awk -v key="\"node_modules/$pkg\": {" '
      index($0, key) { inside = 1; next }
      inside && /"version":/ { gsub(/.*"version": *"|".*/, ""); print; exit }' "$ROOT/$lock")
    found+=("${v:-missing}  $lock $pkg")
  done
done

for file in "${IMAGES[@]}"; do
  tags=$(grep -o 'mcr\.microsoft\.com/playwright:v[^ "]*' "$ROOT/$file" || true)
  [ -n "$tags" ] || { found+=("missing  $file (no mcr.microsoft.com/playwright image)"); continue; }
  while IFS= read -r tag; do
    v=${tag#*:v}
    found+=("${v%-noble}  $file $tag")
  done <<< "$tags"
done

first=${found[0]%%  *}
status=0
for f in "${found[@]}"; do
  [ "${f%%  *}" = "$first" ] || status=1
done
if [ $status -ne 0 ]; then
  echo "Playwright versions differ:"
  printf '  %s\n' "${found[@]}"
  exit 1
fi
echo "Playwright $first everywhere (${#found[@]} places)"

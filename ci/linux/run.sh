#!/bin/sh
# Runs the Linux jobs of both CI workflows locally in Podman, natively on
# arm64, before dispatching CI (ADR 0039). GCC 13 on Ubuntu 24.04 as on the
# runners; x86_64, Windows, macOS packaging and the AppImages stay CI-only.
#
#   ci/linux/run.sh                 library core-59 core-1600 qt-59 qt-1600
#   ci/linux/run.sh core-1600 qt-1600
#   ci/linux/run.sh web-1600        the web job, in the larger web image
#
# The jobs are in ci/linux/jobs.sh. The working tree (with uncommitted
# changes and the ROMs, without .git and what .gitignore ignores) is copied
# into the volume calcu-ci; builds never write into the checkout. Build
# trees and the fetched sharpdx stay in that volume, the compiler cache in
# calcu-ccache. `podman volume rm calcu-ci` starts afresh.
#
# Podman machine: CALCU_PODMAN_MACHINE (default calcu), created once with
#   podman machine init --cpus 12 --memory 16384 --disk-size 100 calcu
# The script starts it when it is stopped; stop it with
#   podman machine stop calcu
set -eu
REPO=$(cd "$(dirname "$0")/../.." && pwd)
HERE="$REPO/ci/linux"
MACHINE=${CALCU_PODMAN_MACHINE:-calcu}
pm() { podman --connection "$MACHINE" "$@"; }

[ $# -gt 0 ] || set -- library core-59 core-1600 qt-59 qt-1600
target=base
for j in "$@"; do case "$j" in web-*) target=web ;; esac; done

# The machine.
if ! state=$(podman machine inspect "$MACHINE" --format '{{.State}}' 2>/dev/null); then
  echo "error: no Podman machine '$MACHINE'; create it with" >&2
  echo "  podman machine init --cpus 12 --memory 16384 --disk-size 100 $MACHINE" >&2
  exit 1
fi
[ "$state" = running ] || podman machine start "$MACHINE" >/dev/null

# The image, rebuilt when the Containerfile changes.
hash=$(shasum -a 256 "$HERE/Containerfile" | cut -c1-12)
image="localhost/calcu-ci-$target:$hash"
if ! pm image exists "$image"; then
  echo "::: building $image"
  pm build --target "$target" -t "$image" -f "$HERE/Containerfile" "$HERE"
  pm image prune -f >/dev/null || true
fi

# The copy: a web job gets its own build/web per product, since each
# product's Emscripten differs and build.sh always uses build/web.
web_mounts=""
for j in "$@"; do
  case "$j" in web-*) web_mounts="$web_mounts -v calcu-ci-$j:/work/build/web" ;; esac
done

# What git ignores (but the ROMs) is left out of the copy, by git's own
# rules; rsync's .gitignore reading gets `!` lines wrong. Excluded paths
# survive --delete in the volume, so its build trees stay.
ignored() {
  # .git, and what only the container makes (and the Mac may not have).
  printf '%s\n' /.git /build/ \
    /products/calc-u-1600/core/Basic/vendor/sharpdx/libsharpdx-linux.a \
    '/products/*/frontends/web/dist/' '/products/*/frontends/web/node_modules/' \
    /library/web/node_modules/ test-results/
  git -C "$REPO" ls-files -oi --exclude-standard --directory \
    | grep -v '^products/[^/]*/roms/' | sed 's|^|/|'
}

# shellcheck disable=SC2086
ignored | pm run --rm -i \
  -v "$REPO":/src:ro -v calcu-ci:/work -v calcu-ccache:/ccache $web_mounts \
  "$image" bash -c '
    set -e
    cat > /tmp/ignored
    rsync -a --delete --exclude-from=/tmp/ignored /src/ /work/
    exec /work/ci/linux/jobs.sh "$@"
  ' jobs "$@"

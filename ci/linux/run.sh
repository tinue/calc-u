#!/bin/sh
# Runs the Linux jobs of both CI workflows locally in Podman, natively on
# arm64, before dispatching CI (ADR 0039). GCC 13 on Ubuntu 24.04 as on the
# runners; x86_64, Windows, macOS packaging and the AppImages stay CI-only.
#
#   ci/linux/run.sh                 library core-59 core-1600 qt-59 qt-1600 help-1600
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

[ $# -gt 0 ] || set -- library core-59 core-1600 qt-59 qt-1600 help-1600
# The web jobs run in the web image, the others in the base image: two runs
# when both are named, so a base job's build tree is never configured in the
# web image (which has node and the browsers the base image lacks).
base_jobs="" web_jobs=""
for j in "$@"; do
  case "$j" in web-*) web_jobs="$web_jobs $j" ;; *) base_jobs="$base_jobs $j" ;; esac
done

# The machine.
if ! state=$(podman machine inspect "$MACHINE" --format '{{.State}}' 2>/dev/null); then
  echo "error: no Podman machine '$MACHINE'; create it with" >&2
  echo "  podman machine init --cpus 12 --memory 16384 --disk-size 100 $MACHINE" >&2
  exit 1
fi
[ "$state" = running ] || podman machine start "$MACHINE" >/dev/null

# The image of a target, rebuilt when the Containerfile changes.
hash=$(shasum -a 256 "$HERE/Containerfile" | cut -c1-12)
image_of() {
  image="localhost/calcu-ci-$1:$hash"
  if ! pm image exists "$image"; then
    echo "::: building $image" >&2
    pm build --target "$1" -t "$image" -f "$HERE/Containerfile" "$HERE" >&2
    pm image prune -f >/dev/null || true
  fi
  echo "$image"
}

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

# run_in <target> <job>...: the copy, then the jobs, in that target's image.
# A web job gets its own build/web per product, since each product's
# Emscripten differs and build.sh always uses build/web.
run_in() {
  target=$1; shift
  image=$(image_of "$target")
  web_mounts=""
  for j in "$@"; do
    case "$j" in web-*) web_mounts="$web_mounts -v calcu-ci-$j:/work/build/web" ;; esac
  done
  # label=disable: SELinux in the Podman machine gave files a container
  # made in a web volume a private category, so the next container could
  # not read them (EACCES); these volumes are build caches of this tool only.
  # shellcheck disable=SC2086
  ignored | pm run --rm -i --security-opt label=disable \
    -v "$REPO":/src:ro -v calcu-ci:/work -v calcu-ccache:/ccache $web_mounts \
    "$image" bash -c '
      set -e
      cat > /tmp/ignored
      rsync -a --delete --exclude-from=/tmp/ignored /src/ /work/
      exec /work/ci/linux/jobs.sh "$@"
    ' jobs "$@"
}

status=0
# shellcheck disable=SC2086
[ -z "$base_jobs" ] || run_in base $base_jobs || status=1
# shellcheck disable=SC2086
[ -z "$web_jobs" ] || run_in web $web_jobs || status=1
exit $status

# The common part of the products' ROM fetch scripts
# (products/<product>/roms/fetch_roms.sh), sourced by them: each product's
# script is its fetch list (names, md5 and sources of its full set) and
# calls fetch_if_needed for each file. ROMs are not in this repository (the
# manufacturers' copyrighted firmware), so this is how a fresh clone (or CI)
# gets a working roms/ directory.
#
# Idempotent: a file that is there with its known md5 is skipped, so
# running again after a partial or interrupted fetch is safe and cheap.
#
# Sourced with the product's roms/ directory as the working directory; the
# files go into ROMS_DIR (that directory).

ROMS_DIR=.

log() { echo "fetch_roms.sh: $*" >&2; }
die() { log "error: $*"; exit 1; }
# md5 (BSD/macOS) vs md5sum (Linux) -- print just the hex digest.
md5_of() {
  if command -v md5 >/dev/null 2>&1; then
    md5 -q "$1"
  else
    md5sum "$1" | awk '{print $1}'
  fi
}

# fetch_if_needed DEST_PATH EXPECTED_MD5 URL... -- downloads the URLs in
# order and stores their concatenation (more than one for a ROM kept
# upstream as one file per bank).
fetch_if_needed() {
  dest=$1
  expected_md5=$2
  shift 2

  # A file left over with different letter case (e.g. a stale ...-old.BIN)
  # would match `-f "$dest"` on a case-insensitive filesystem and never get
  # renamed to the name the lookup expects -- drop it first.
  for existing in "$(dirname "$dest")"/*; do
    [ "$existing" != "$dest" ] && [ "$(printf %s "$existing" | tr 'A-Z' 'a-z')" = "$(printf %s "$dest" | tr 'A-Z' 'a-z')" ] && rm -f "$existing"
  done

  if [ -f "$dest" ] && [ "$(md5_of "$dest")" = "$expected_md5" ]; then
    log "up to date: $dest"
    return 0
  fi

  tmp="$dest.tmp$$"
  : > "$tmp"
  for url in "$@"; do
    log "fetching $dest <- $url"
    if ! curl -fsSL "$url" >> "$tmp"; then
      rm -f "$tmp"
      die "download failed: $url"
    fi
  done

  actual_md5=$(md5_of "$tmp")
  if [ "$actual_md5" != "$expected_md5" ]; then
    rm -f "$tmp"
    die "checksum mismatch for $dest (got $actual_md5, expected $expected_md5) -- upstream file may have changed, update the product's fetch list"
  fi
  mv "$tmp" "$dest"
}

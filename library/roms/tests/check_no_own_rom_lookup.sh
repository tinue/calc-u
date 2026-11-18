#!/bin/sh
# No other part of a product looks for a ROM file. Fails when a product
# source outside the allowed files
#   - uses the old ROM loading APIs (directoryReader, ResourceReader,
#     TIResourceReader, BundledRoms, loadRom, loadModule from a file), or
#   - opens a file (ifstream, fopen, QFile, forResource:, fetch) on a line
#     that names a ROM file or a ROM extension.
# The ROM lookup itself (library/roms/) is not a product source.
#
# Usage: check_no_own_rom_lookup.sh <product-dir> [allowed-path ...]
#   allowed-path: relative to <product-dir>; a file that may do it (the
#   product's lookUpRoms() and its catalogue data, a frontend's Bundle, and
#   the file that reads the host drive's ROM, a bundled file Calc-U-1600
#   reads outside the lookup).
#   tests/ and documentation are not checked.
# CTest CalcU59NoOwnRomLookup, CalcU1600NoOwnRomLookup.
set -eu
[ $# -ge 1 ] || { echo "usage: $0 <product-dir> [allowed-path ...]" >&2; exit 2; }
cd "$1"
shift

apis='(directoryReader|ResourceReader|TIResourceReader|BundledRoms|BundledRomCatalog|parseModuleTxt|RomLoader)'
opens='(ifstream|fopen|QFile|forResource|fetch[ (]|open\()'
names='(\.ROM\b|\.rom\b|\.rom\.yaml|\.bin"|TMC05[0-9]{2}[A-Z]?(-CONST-K)?\.txt|CD240[01](-CONST-K)?\.txt|PC-?1[56]00[^"]*\.(ROM|bin)|CE-15[08]\.ROM)'

dirs=""
for d in core app frontends tools; do [ -d "$d" ] && dirs="$dirs $d"; done
[ -n "$dirs" ] || { echo "no product sources under $(pwd)" >&2; exit 2; }

# shellcheck disable=SC2086
all=$( (grep -rnE "$apis" $dirs \
          --include='*.cpp' --include='*.hpp' --include='*.h' --include='*.mm' --include='*.m' \
          --include='*.swift' --include='*.js' --include='*.jsx' --include='*.py' 2>/dev/null || true;
        grep -rnE "$opens" $dirs \
          --include='*.cpp' --include='*.hpp' --include='*.h' --include='*.mm' --include='*.m' \
          --include='*.swift' --include='*.js' --include='*.jsx' --include='*.py' 2>/dev/null \
          | grep -E "$names" || true) \
      | grep -vE '/(build|dist|node_modules|DerivedData|vendor|__pycache__)/' \
      | grep -vE ':[0-9]+:[[:space:]]*(//|#|\*|/\*)' || true)

hits=""
if [ -n "$all" ]; then
  hits=$(printf '%s\n' "$all" | while IFS= read -r line; do
    file=${line%%:*}
    ok=0
    for allowed in "$@"; do
      [ "$file" = "$allowed" ] && ok=1
      [ "${file#"$allowed"/}" != "$file" ] && ok=1
    done
    [ $ok -eq 1 ] || printf '%s\n' "$line"
  done)
fi

if [ -n "$hits" ]; then
  echo "a ROM file is looked for outside the ROM lookup:"
  printf '%s\n' "$hits"
  exit 1
fi
echo "no own ROM lookup: ok"

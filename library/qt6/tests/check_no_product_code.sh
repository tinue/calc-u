#!/usr/bin/env bash
# LIB-14 L11 (design §6; spec AC-22, Technical Requirements; ADR 0023,
# frontend.md I-4): the shell's rules and both component sets hold no code of
# a product: no include from products/ and no model, peripheral or product
# name in the code of library/shell, library/qt6 and library/web/components.
# Comments may name the products they were taken from; code may not. CTest
# LibraryShellNoProduct (Frequent; not on Windows).
#
#   check_no_product_code.sh [<library folder>]
set -euo pipefail

LIB="${1:-$(cd "$(dirname "$0")/../.." && pwd)}"
NAMES='TI-5[89]|TI-58C|PC-1[56]00A?|PC-100C|CE-1[56][0-9]|CE-1600P|Calc-U-(59|1600)|calc-u-(59|1600)|cu59::|cu1600::|c59\.|c1600\.'

found=0
while IFS= read -r -d '' file; do
  # Code only: a line without its // comment; no lines of /* */ blocks that
  # start with *, no # comment lines (but #include).
  hits=$(awk -v names="$NAMES" '
    {
      line = $0
      if (line ~ /^[[:space:]]*(\*|\/\*)/) next
      if (line ~ /^[[:space:]]*#/ && line !~ /^[[:space:]]*#[[:space:]]*include/) next
      sub(/[[:space:]]\/\/.*$/, "", line)
      sub(/^[[:space:]]*\/\/.*$/, "", line)
      if (line ~ /#[[:space:]]*include[[:space:]]*[<"]products\// || line ~ names) printf "%d: %s\n", NR, $0
    }' "$file")
  if [ -n "$hits" ]; then
    echo "$file:"
    echo "$hits"
    found=1
  fi
done < <(find "$LIB/shell" "$LIB/qt6" "$LIB/web/components" -type f \
           \( -name '*.cpp' -o -name '*.hpp' -o -name '*.js' -o -name '*.jsx' -o -name '*.css' -o -name '*.txt' \) -print0)

if [ "$found" -ne 0 ]; then
  echo "error: product code in the shell or a component set (LIB-14 AC-22)." >&2
  exit 1
fi
echo "No product code in library/shell, library/qt6, library/web/components."

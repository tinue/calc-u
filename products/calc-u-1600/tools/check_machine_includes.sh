#!/bin/sh
# C1600-1, Technical Requirements ("No model above the machine"; design
# T13, D-9): nothing above the core includes or names a machine class.
# Outside core/ (app/, frontends/, tools/, dev/; tests/ and docs/ are not
# checked) no file
#   1. includes PC1500Machine.hpp or PC1600Machine.hpp, or a core header that
#      includes one of them, directly or through other core headers;
#   2. names PC1500Machine or PC1600Machine outside a // comment.
# The app layer reaches the machine as calcu::machine::Machine
# (core/Machine/). Run from products/calc-u-1600 (CTest
# CalcU1600MachineIncludeCheck, group Frequent; CI runs it with the core
# tests).
set -eu
cd "$(dirname "$0")/.."
status=0

sources() {
  find app frontends tools dev -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.mm' -o -name '*.m' \) \
    2>/dev/null | grep -vE '/(build|dist|node_modules|vendor|DerivedData)/' || true
}

# The core headers that bring a machine class in, by file name: the two
# class headers, then every core header that includes one of the set, until
# nothing more is added.
set_file=$(mktemp)
trap 'rm -f "$set_file" "$set_file.new"' EXIT
printf '%s\n' PC1500Machine.hpp PC1600Machine.hpp > "$set_file"
while :; do
  pattern=$(sed 's/\./\\./g' "$set_file" | paste -sd'|' -)
  find core -type f -name '*.hpp' | while read -r h; do
    if grep -qE "^[[:space:]]*#[[:space:]]*include[[:space:]]*[<\"]([^\">]*/)?($pattern)[\">]" "$h"; then
      basename "$h"
    fi
  done | cat "$set_file" - | sort -u > "$set_file.new"
  if cmp -s "$set_file" "$set_file.new"; then break; fi
  mv "$set_file.new" "$set_file"
done

pattern=$(sed 's/\./\\./g' "$set_file" | paste -sd'|' -)
hits=$(sources | while read -r f; do
  grep -nE "^[[:space:]]*#[[:space:]]*include[[:space:]]*[<\"]([^\">]*/)?($pattern)[\">]" "$f" | sed "s|^|$f:|"
done)
if [ -n "$hits" ]; then
  echo "a machine class is included outside core/ (directly or through these core headers: $(paste -sd' ' "$set_file")):"
  echo "$hits"
  status=1
fi

names=$(sources | while read -r f; do
  sed 's|//.*$||' "$f" | grep -nwE 'PC1500Machine|PC1600Machine' | sed "s|^|$f:|"
done)
if [ -n "$names" ]; then
  echo "a machine class is named outside core/:"
  echo "$names"
  status=1
fi

[ $status -eq 0 ] && echo "machine includes: ok"
exit $status

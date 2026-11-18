#!/usr/bin/env bash
# Source check used by both products: no platform store for settings in a
# product's frontends. Lists every use of QSettings, UserDefaults (also
# NSUserDefaults and @AppStorage) and localStorage in the source files under
# <tree>, except those an allow-list names, and fails when one is left.
#
#   check_platform_stores.sh <tree> [<allow-list>]
#
# The allow-list has one entry per line, `<path under tree>: <text>`; a use
# is allowed when it is in that file and its line contains <text>. Blank
# lines and lines starting with `#` are comments. Each entry says in a
# comment above it why the use is not a setting.
set -euo pipefail

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
  echo "usage: $0 <tree> [<allow-list>]" >&2
  exit 2
fi
tree="${1%/}"
allow="${2:-}"
if [ ! -d "$tree" ]; then
  echo "error: no folder $tree" >&2
  exit 2
fi
if [ -n "$allow" ] && [ ! -f "$allow" ]; then
  echo "error: no allow-list $allow" >&2
  exit 2
fi

found=0
while IFS= read -r hit; do
  [ -n "$hit" ] || continue
  file="${hit%%:*}"
  rest="${hit#*:}"
  line="${rest#*:}"
  rel="${file#"$tree"/}"
  allowed=0
  if [ -n "$allow" ]; then
    while IFS= read -r entry || [ -n "$entry" ]; do
      case "$entry" in ''|'#'*) continue ;; esac
      apath="${entry%%:*}"
      atext="${entry#*:}"
      atext="${atext# }"
      if [ "$apath" = "$rel" ] && [[ "$line" == *"$atext"* ]]; then
        allowed=1
        break
      fi
    done < "$allow"
  fi
  if [ "$allowed" -eq 0 ]; then
    echo "platform store: $rel:${rest%%:*}: ${line#"${line%%[![:space:]]*}"}"
    found=1
  fi
done < <(grep -rnE 'QSettings|UserDefaults|@AppStorage|localStorage' "$tree" \
           --include='*.cpp' --include='*.hpp' --include='*.h' --include='*.mm' --include='*.m' \
           --include='*.swift' --include='*.js' --include='*.mjs' --include='*.ts' --include='*.html' \
           --exclude-dir=node_modules --exclude-dir=dist --exclude-dir=build --exclude-dir=vendor || true)

if [ "$found" -ne 0 ]; then
  echo "settings belong in the product's settings file; allow a use that is not a setting in the allow-list" >&2
  exit 1
fi
echo "no platform store for settings under $tree"

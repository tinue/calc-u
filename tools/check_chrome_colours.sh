#!/bin/sh
# The chrome's colours live in the product's app/ (TIChrome, SharpChrome)
# and every GUI reads them: no colour literal (#rrggbb, its URL form
# %23rrggbb, rgb( / rgba( / hsl( / hsla( with numbers, QColor( /
# QColor::fromRgb( with numbers, Qt::white and the other named Qt colours,
# a named CSS colour as the value of a colour property (`color:`,
# `background:`, `border…:`, `outline…:`, `fill`, `stroke`) or of a custom
# property (`--calcu-x: gray`, the CSS's fallbacks are system colours),
# SwiftUI Color(red: / Color(white:) in a
# frontend's code or the library's component sets (web/, qt6/), but where
# the folder's allow-list says why. Comment lines are not code.
# Usage: check_chrome_colours.sh <product folder | library folder>; the
# allow-list is <folder>/tools/chrome_colours_allowed.txt, one entry per
# line: a path below the folder (a file or a folder) and, optionally after
# a tab, an extended regular expression the line must match; # starts a
# comment. Test code (a tests/ folder) is not checked. Exit 1 with the
# lines found.
set -u
product=${1:?product folder}
cd "$product" || exit 2
allowed=tools/chrome_colours_allowed.txt
found=$(/usr/bin/grep -rnE \
    '(#|%23)[0-9a-fA-F]{3}([0-9a-fA-F]{3})?([0-9a-fA-F]{2})?\b|rgba?\(\s*[0-9]|QColor\(\s*[0-9]|QColor::fromRgbF?\(\s*[0-9]|Qt::(white|black|red|green|blue|gray|darkGray|lightGray|yellow)\b|hsla?\(\s*[0-9]|(color|background|border[a-z-]*|outline[a-z-]*|fill|stroke|--[a-zA-Z0-9-]+)["'"'"']?\s*[:=]\s*["'"'"']?([^;{}"'"'"'()]*\s)?(white|black|gray|grey|silver|red|green|blue|yellow)([;'"'"'"} !,]|$)|Color\((red|white):' \
    --include='*.cpp' --include='*.hpp' --include='*.h' --include='*.mm' --include='*.swift' \
    --include='*.js' --include='*.jsx' --include='*.css' --include='*.html' \
    frontends/qt6/app frontends/web/src frontends/web/app frontends/web/calc.css frontends/web/index.html \
    frontends/swift/App frontends/swift/Bridge web/components web/src web/app qt6 2>/dev/null \
    | /usr/bin/grep -vE '^[^/:]*(/[^/:]+)*/tests/' \
    | /usr/bin/grep -vE '^[^:]+:[0-9]+:\s*(//|\*|/\*|#)')
if [ -n "$found" ] && [ -f "$allowed" ]; then
    found=$(printf '%s\n' "$found" | awk -v list="$allowed" '
        BEGIN {
            while ((getline entry < list) > 0) {
                if (entry ~ /^#/ || entry == "") continue
                split(entry, f, "\t"); n++; path[n] = f[1]; pattern[n] = f[2]
            }
        }
        {
            file = $0; sub(/:.*/, "", file)
            code = $0; sub(/^[^:]*:[0-9]+:/, "", code)
            for (i = 1; i <= n; i++)
                if ((file == path[i] || index(file, path[i] "/") == 1) && (pattern[i] == "" || code ~ pattern[i])) next
            print
        }')
fi
[ -n "$found" ] || exit 0
echo "A colour literal in a frontend; its colour belongs in the product's chrome (app/):"
echo "$found"
exit 1

#!/usr/bin/env bash
# Warn when a branch changes one GUI but not the others (CLAUDE.md
# § "Frontend Parity"): frontends/swift/App/ (Swift), frontends/qt6/app/ (Qt), frontends/web/src/ +
# frontends/web/app/ + frontends/web/calc.css (web). Touching docs/developer/Frontend-Parity.md counts as
# acknowledging the gap (a Pending row). A Swift + Qt change without the web
# is not flagged: much of the apps is outside the web's scope, and the
# table's Web column says which.
#
#   tools/check_parity.sh [base]     # base defaults to main
#
# Exit status 1 = one-sided change without a parity-table update.
set -euo pipefail
BASE="${1:-main}"
changed="$(git diff --name-only "$(git merge-base "$BASE" HEAD)"...HEAD; git diff --name-only HEAD)"

swift=$(printf '%s\n' "$changed" | grep -E '^products/calc-u-59/frontends/swift/App/' | grep -vE '^products/calc-u-59/frontends/swift/App/(Assets\.xcassets|AppIcon\.icon)/' || true)
qt=$(printf '%s\n' "$changed" | grep -E '^products/calc-u-59/frontends/qt6/app/' || true)
web=$(printf '%s\n' "$changed" | grep -E '^products/calc-u-59/frontends/web/(src/|app/|calc\.css$)' || true)
table=$(printf '%s\n' "$changed" | grep -E '^products/calc-u-59/docs/developer/Frontend-Parity\.md$' || true)

if [ -n "$web" ] && [ -z "$swift$qt" ] && [ -z "$table" ]; then
    echo "Web client changed without an app change or a Frontend-Parity.md update:"
    printf '  %s\n' $web
    exit 1
fi
if [ -n "$swift" ] && [ -z "$qt" ] && [ -z "$table" ]; then
    echo "Swift GUI changed without a Qt6 change or a Frontend-Parity.md update:"
    printf '  %s\n' $swift
    exit 1
fi
if [ -n "$qt" ] && [ -z "$swift" ] && [ -z "$table" ]; then
    echo "Qt6 GUI changed without a Swift change or a Frontend-Parity.md update:"
    printf '  %s\n' $qt
    exit 1
fi
echo "parity check: ok"

#!/bin/sh
# Parses every preset in the repository (nothing is booted) and reports the
# ones that don't parse. Run it after a change to the preset format.
# Usage: tools/check_presets.sh   (builds headless/pc1600_cli, tools/build_tool.sh)
# CTest's CalcU1600CheckPresets (Frequent group) passes its own build as
# PC1600_CLI.
set -e
cd "$(dirname "$0")/.."
CLI="${PC1600_CLI:-}"
if [ -z "$CLI" ]; then
  tools/build_tool.sh pc1600_cli > /dev/null
  CLI=./headless/pc1600_cli
fi
# The VS Code templates hold {{name}} placeholders; they are checked by the
# extension's own tests once filled in.
git ls-files '*.pc1500' '*.pc1500a' '*.pc1600' | grep -v '^vscode/calcu1600-debug/templates/' |
  tr '\n' '\0' | xargs -0 "$CLI" --check-preset

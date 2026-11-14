#!/bin/sh
# Parses every preset in the repository (nothing is booted) and reports the
# ones that don't parse. Run it after a change to the preset format.
# Usage: tools/check_presets.sh   (builds headless/pc1600_cli, tools/build_tool.sh)
set -e
cd "$(dirname "$0")/.."
tools/build_tool.sh pc1600_cli > /dev/null
# The VS Code templates hold {{name}} placeholders; they are checked by the
# extension's own tests once filled in.
git ls-files '*.pc1500' '*.pc1500a' '*.pc1600' | grep -v '^vscode/calcu1600-debug/templates/' |
  tr '\n' '\0' | xargs -0 ./headless/pc1600_cli --check-preset

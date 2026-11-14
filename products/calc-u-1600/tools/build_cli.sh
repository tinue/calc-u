#!/bin/sh
# Builds the headless CLI harness. Usage: tools/build_cli.sh && ./headless/pc1500_cli roms/PC-1500_A04.ROM
# The target and its sources: the product's CMakeLists.txt (tools/build_tool.sh).
exec "$(dirname "$0")/build_tool.sh" pc1500_cli

#!/bin/sh
# Builds the headless PC-1600 CLI harness. Usage:
#   tools/build_pc1600_cli.sh && ./headless/pc1600_cli roms/PC1600-P0-B0-new.bin roms/PC1600-P1-B0-new.bin
#   tools/build_pc1600_cli.sh && ./headless/pc1600_cli --preset examples/foo.pc1600 --dump-basic
# The target and its sources: the product's CMakeLists.txt (tools/build_tool.sh).
exec "$(dirname "$0")/build_tool.sh" pc1600_cli

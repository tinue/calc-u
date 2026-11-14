#!/bin/sh
# Builds the headless PC-1600 + CE-1600P plotter probe. Boots a
# `plotter: CE-1600P` preset, runs its script, and dumps the plotter
# mechanism's pen/motor/colour events per preset step. Investigation aid
# for the "pen colour drifts out of sync on OFF/ON" issue -- see
# MinorIssues.md.
#
# Usage:
#   tools/build_pc1600_plotter_probe.sh && \
#     ./headless/pc1600_plotter_probe examples/plotter/ce1600p-text-and-frame.pc1600
# The target and its sources: the product's CMakeLists.txt (tools/build_tool.sh).
exec "$(dirname "$0")/build_tool.sh" pc1600_plotter_probe

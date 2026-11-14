#!/bin/sh
# Builds the throw-away OFF-key ROM-path probe (see
# tools/pc1600_power_probe.cpp).
# PC1600_POWER_PROBE lights up the instrumentation hooks in
# PC1600Memory::writeIO / PC1600SubCpu::command; only the probes' copy of
# the core (calcu_core_probe) defines it.
#
#   tools/build_pc1600_power_probe.sh && ./headless/pc1600_power_probe roms
# The target and its sources: the product's CMakeLists.txt (tools/build_tool.sh).
exec "$(dirname "$0")/build_tool.sh" pc1600_power_probe

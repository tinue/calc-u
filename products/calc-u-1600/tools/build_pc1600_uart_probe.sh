#!/bin/sh
# Builds the throw-away S2:-RAM-disk / UART-handshake ROM-path probe (see
# tools/pc1600_uart_probe.cpp). Same PC1600_POWER_PROBE instrumentation
# hooks as the power probe (calcu_core_probe).
#
#   tools/build_pc1600_uart_probe.sh && ./headless/pc1600_uart_probe roms
# The target and its sources: the product's CMakeLists.txt (tools/build_tool.sh).
exec "$(dirname "$0")/build_tool.sh" pc1600_uart_probe

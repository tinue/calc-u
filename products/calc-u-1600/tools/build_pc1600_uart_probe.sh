#!/bin/sh
# Builds the throw-away S2:-RAM-disk / UART-handshake ROM-path probe (see
# tools/pc1600_uart_probe.cpp). Same -DPC1600_POWER_PROBE instrumentation
# hooks as the power probe; no other build defines it.
#
#   tools/build_pc1600_uart_probe.sh && ./headless/pc1600_uart_probe roms
set -e
cd "$(dirname "$0")/.."
mkdir -p headless
clang++ -std=c++20 -Wall -Wextra -O2 -DPC1600_POWER_PROBE \
  core/CPU/LH5801/LH5801.cpp \
  core/CPU/SC7852/SC7852.cpp \
  core/CPU/LH5803/LH5803Memory.cpp \
  core/CPU/LH5803/LH5803SharedMemory.cpp \
  core/Audio/PiezoSampler.cpp \
  core/PC1600/PC1600Memory.cpp \
  core/PC1600/PC1600Keyboard.cpp \
  core/PC1600/PC1600Display.cpp \
  core/PC1600/PC1600SubCpu.cpp \
  core/PC1600/TC8576F.cpp \
  core/Serial/PtySerialLink.cpp \
  core/PC1600/PC1600Machine.cpp \
  core/PC1600/PC1600BasicTyper.cpp \
  core/PC1600/PC1600ProgramPlacement.cpp \
  core/PC1500/PC1500TraceFile.cpp \
  tools/pc1600_uart_probe.cpp \
  -o headless/pc1600_uart_probe

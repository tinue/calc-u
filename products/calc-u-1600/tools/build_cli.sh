#!/bin/sh
# Builds the headless CLI harness. Usage: tools/build_cli.sh && ./headless/pc1500_cli roms/PC-1500_A04.ROM
set -e
cd "$(dirname "$0")/.."
mkdir -p headless
clang++ -std=c++20 -Wall -Wextra -O2 \
  core/CPU/LH5801/LH5801.cpp \
  core/Audio/PiezoSampler.cpp \
  core/PC1500/PC1500Memory.cpp \
  core/PC1500/PC1500Machine.cpp \
  core/Debug/Inspect/Inspector.cpp \
  core/Debug/Inspect/TextTable.cpp \
  core/Debug/Inspect/HexDump.cpp \
  core/Debug/Inspect/PC1500Inspector.cpp \
  core/PC1500/PC1500Keyboard.cpp \
  core/PC1500/PC1500Display.cpp \
  core/PC1500/Upd1990ac.cpp \
  core/Preset/PresetFile.cpp \
  core/Preset/PresetRunner.cpp \
  core/MachineCodeFile.cpp \
  core/ProgramFile.cpp \
  core/PC1500/PC1500BasicTyper.cpp \
  core/PC1500/PC1500BasicLoader.cpp \
  core/PC1500/PC1500PresetLoader.cpp \
  core/PC1500/PC1500MachineCodeLoader.cpp \
  core/Display/LcdScreenshot.cpp \
  core/Display/LcdText.cpp \
  core/Basic/BasicProgramSource.cpp \
  core/PC1500/PC1500TraceFile.cpp \
  core/Serial/PtySerialLink.cpp \
  tools/pc1500_cli.cpp \
  core/Basic/vendor/sharpdx/libsharpdx.a \
  -o headless/pc1500_cli

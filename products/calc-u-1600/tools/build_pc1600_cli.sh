#!/bin/sh
# Builds the headless PC-1600 CLI harness. Usage:
#   tools/build_pc1600_cli.sh && ./headless/pc1600_cli roms/PC1600-P0-B0-new.bin roms/PC1600-P1-B0-new.bin
#   tools/build_pc1600_cli.sh && ./headless/pc1600_cli --preset examples/foo.pc1600 --dump-basic
set -e
cd "$(dirname "$0")/.."
mkdir -p headless
clang++ -std=c++20 -Wall -Wextra -O2 \
  core/CPU/LH5801/LH5801.cpp \
  core/CPU/SC7852/SC7852.cpp \
  core/CPU/LH5803/LH5803Memory.cpp \
  core/CPU/LH5803/LH5803SharedMemory.cpp \
  core/Preset/PresetFile.cpp \
  core/Preset/PresetRunner.cpp \
  core/MachineCodeFile.cpp \
  core/ProgramFile.cpp \
  core/PC1500/PC1500Keyboard.cpp \
  core/PC1500/PC1500TraceFile.cpp \
  core/Audio/PiezoSampler.cpp \
  core/PC1600/PC1600Memory.cpp \
  core/PC1600/PC1600SubCpu.cpp \
  core/PC1600/TC8576F.cpp \
  core/Serial/PtySerialLink.cpp \
  core/PC1600/PC1600Display.cpp \
  core/PC1600/PC1600Keyboard.cpp \
  core/PC1600/PC1600Machine.cpp \
  core/Debug/BasicPointerTable.cpp \
  core/Debug/Inspect/Inspector.cpp \
  core/Debug/Inspect/TextTable.cpp \
  core/Debug/Inspect/HexDump.cpp \
  core/Debug/Inspect/PC1600Inspector.cpp \
  core/PC1600/PC1600BasicTyper.cpp \
  core/PC1600/PC1600BasicLoader.cpp \
  core/PC1600/PC1600ProgramPlacement.cpp \
  core/PC1600/PC1600MachineCodeLoader.cpp \
  core/PC1600/PC1600PresetLoader.cpp \
  core/Display/LcdScreenshot.cpp \
  core/Display/LcdText.cpp \
  core/Basic/BasicProgramSource.cpp \
  tools/pc1600_cli.cpp \
  core/Basic/vendor/sharpdx/libsharpdx.a \
  -o headless/pc1600_cli

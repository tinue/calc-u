#!/bin/sh
# Builds the PC-1600 loader-matrix harness into headless/loader-matrix/.
# Usage: dev/loader-matrix/build_pc1600.sh
set -e
cd "$(dirname "$0")/../.."
mkdir -p headless/loader-matrix
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
  core/PC1600/PC1600Display.cpp \
  core/PC1600/PC1600Keyboard.cpp \
  core/PC1600/PC1600Machine.cpp \
  core/PC1600/PC1600BasicTyper.cpp \
  core/PC1600/PC1600BasicLoader.cpp \
  core/PC1600/PC1600ProgramPlacement.cpp \
  core/PC1600/PC1600MachineCodeLoader.cpp \
  core/PC1600/PC1600PresetLoader.cpp \
  core/Display/LcdScreenshot.cpp \
  core/Display/LcdText.cpp \
  core/Basic/BasicProgramSource.cpp \
  dev/loader-matrix/pc1600_loader_matrix.cpp \
  core/Basic/vendor/sharpdx/libsharpdx.a \
  -o headless/loader-matrix/pc1600_loader_matrix

#!/bin/sh
# Builds the loader-matrix harness into headless/loader-matrix/.
# Usage: dev/loader-matrix/build.sh
set -e
cd "$(dirname "$0")/../.."
mkdir -p headless/loader-matrix
clang++ -std=c++20 -Wall -Wextra -O2 \
  core/CPU/LH5801/LH5801.cpp \
  core/Audio/PiezoSampler.cpp \
  core/PC1500/PC1500Memory.cpp \
  core/PC1500/PC1500Machine.cpp \
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
  dev/loader-matrix/pc1500_loader_matrix.cpp \
  core/Basic/vendor/sharpdx/libsharpdx.a \
  -o headless/loader-matrix/pc1500_loader_matrix

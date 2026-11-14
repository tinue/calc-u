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
set -e
cd "$(dirname "$0")/.."
mkdir -p headless
clang++ -std=c++20 -Wall -Wextra -O1 \
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
  core/PC1500/PC1500PresetLoader.cpp \
  core/PC1500/PC1500TraceFile.cpp \
  core/PC1600/PC1600Memory.cpp \
  core/PC1600/PC1600Keyboard.cpp \
  core/PC1600/PC1600Display.cpp \
  core/PC1600/PC1600SubCpu.cpp \
  core/PC1600/TC8576F.cpp \
  core/CPU/SC7852/SC7852.cpp \
  core/CPU/LH5803/LH5803Memory.cpp \
  core/CPU/LH5803/LH5803SharedMemory.cpp \
  core/PC1600/PC1600Machine.cpp \
  core/PC1600/PC1600BasicTyper.cpp \
  core/PC1600/PC1600MachineCodeLoader.cpp \
  core/PC1600/PC1600PresetLoader.cpp \
  core/Display/LcdScreenshot.cpp \
  core/Display/LcdText.cpp \
  core/Basic/BasicProgramSource.cpp \
  core/PC1500/PC1500BasicLoader.cpp \
  core/PC1600/PC1600BasicLoader.cpp \
  core/PC1600/PC1600ProgramPlacement.cpp \
  core/PC1500/PC1500MachineCodeLoader.cpp \
  tools/pc1600_plotter_probe.cpp \
  core/Basic/vendor/sharpdx/libsharpdx.a \
  -o headless/pc1600_plotter_probe

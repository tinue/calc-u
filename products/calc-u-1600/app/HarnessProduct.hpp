#pragma once
#include "harness/Product.hpp"
#include <memory>

// Calc-U-1600 in the harness `calcu` (LIB-11 design §4, Part D; M-28, M-51):
// the library's app session on a Sharp machine with the product's app
// layer, as the frontends host it. Each run builds its machine through
// app/SharpAppPart (LIB-11 D-6, D-11: prepare() on the run's settings and
// folders), whose library parts are the run's (libraryParts()); the
// product adds its display text part (app/LcdTextPart: `lcd` with the cursor
// cell, AC-19) and the serial ports' scripts (app/SerialScripts), and its
// commands under `pc1600` (app/SharpHarnessCommands: C1600-2, C1600-5). The
// ROMs are looked up in roms/ through sharp::lookUpRoms, the module
// definitions in frontends/qt6/resources/cards/, the presets in examples/,
// the host drive's ROM in firmware/pc1600-hostdrive/ (AC-20).

namespace cu1600 {

/// The factory registered with calcu_harness_add_product (CMakeLists.txt).
std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct();

} // namespace cu1600

#pragma once
#include "harness/Product.hpp"
#include <memory>

// Calc-U-1600 in the harness `calcu`:
// the library's app session on a Sharp machine with the product's app
// layer, as the frontends host it. Each run builds its machine through
// app/SharpAppPart (prepare() on the run's settings and
// folders), whose library parts are the run's (libraryParts()); the
// product adds its display text part (app/LcdTextPart: `display-text` with the cursor
// cell) and the serial ports' scripts (app/SerialScripts), and its
// commands under `c1600` (app/SharpHarnessCommands). The
// ROMs are looked up in roms/ through sharp::lookUpRoms, the module
// definitions in frontends/qt6/resources/cards/, the presets in examples/,
// the host drive's ROM in firmware/pc1600-hostdrive/.

namespace cu1600 {

/// The factory registered with calcu_harness_add_product (CMakeLists.txt).
std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct();

} // namespace cu1600

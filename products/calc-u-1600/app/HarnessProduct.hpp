#pragma once
#include "harness/Product.hpp"
#include <memory>

// Calc-U-1600 in the harness `calcu` (LIB-11 design §4; M-28): the library's
// app session on a Sharp machine through the machine interface. The
// product's part of the session (SharpAppPart, M-31) does not join it yet
// (LIB-11 D-6, M-51): the harness's machines are its own
// SharpProductMachines (LIB-18 §10), its presets SharpPresetRules over
// examples/ (LIB-6 §10). Adapter A-4, the font of `lcd` from the model's ROM
// (core/PC1500/PC1500LcdText.hpp, core/PC1600/PC1600LcdText.hpp) until M-51.
// The settings declaration and the ROM lookup are the product's
// (app/Settings.hpp; sharp::lookUpRoms over roms/).
// The product commands of C1600-2 (M-44): `pc1600 file-kind`, `load-basic`,
// `type-basic`, `load-code` on app/SharpLoaderPart; the rest with C1600-5
// (M-51). No cursor cell in `lcd` (D-7).

namespace shared {

/// The factory registered with calcu_harness_add_product (CMakeLists.txt).
std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct();
/// The same with the ROMs of `romFolder` (the CLIs' `roms`, relative to
/// where they run; LIB-11 AC-20) in place of the product's roms/.
std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct(std::string romFolder);

} // namespace shared

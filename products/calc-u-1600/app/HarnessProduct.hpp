#pragma once
#include "harness/Product.hpp"
#include <memory>

// Calc-U-1600 in the harness `calcu` (LIB-11 design §4; M-28): the library's
// app session on a Sharp machine through the machine interface. The
// product's AppSession and MachineSession are not used; its app layer joins
// in M-31 (D-6). Adapters (design §5): A-1, the start (the PC-1500A unless
// [startup] model names another, its default variant, nothing plugged in,
// then the first cold boot); A-3, a settings declaration of [startup] model
// only, until LIB-9 1600 (M-30); A-4, the font of `lcd` from the model's ROM
// (core/PC1500/PC1500LcdText.hpp, core/PC1600/PC1600LcdText.hpp) until M-51.
// The ROMs are looked up with the library's lookup over the folders the
// tests use, until LIB-12 1600 (M-27) gives the product its lookUpRoms().
// No product commands until C1600-5 (M-51); no cursor cell in `lcd` (D-7).

namespace shared {

/// The factory registered with calcu_harness_add_product (CMakeLists.txt).
std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct();

} // namespace shared

#pragma once
#include "harness/Product.hpp"
#include <memory>

// Calc-U-59 in the harness `calcu` (LIB-11 design §4; M-28): the library's
// app session on a TI machine through the machine interface. The product's
// own session classes are not used; its app layer joins in M-33 (D-6).
// Adapters (design §5): A-1, the start (TI-59 unless [startup] model names
// another, its default variant, the PC-100C plugged in, no module, as
// 2.3.0's core). The settings declaration and the ROM lookup are the
// product's (app/Settings.hpp, app/ProductRoms.hpp). No product commands
// until C59-9 (M-59).

namespace cu59 {

/// The factory registered with calcu_harness_add_product (CMakeLists.txt).
std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct();

/// The rule of `lcd` for one position (AC-10; AC-16 is checked in M-59): a
/// digit, -, °, " or ' from the lit segments A–G (core/TISegments.hpp), a
/// blank for a position not driven, then "." for a lit decimal point, as
/// display() of tools/headless/ti59.py.
std::string tiPositionText(const calcu::machine::SegmentPosition&);

} // namespace cu59

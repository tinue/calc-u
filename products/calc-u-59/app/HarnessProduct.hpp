#pragma once
#include "harness/Product.hpp"
#include <memory>

// Calc-U-59 in the harness `calcu` (LIB-11 design §4, §11.3; M-28, M-59): the
// library's app session on a TI machine with the product's app layer,
// TIAppPart, as the frontends host it (built in prepare() on the run's
// settings and folders; its library parts are the run's), the product
// commands under `ti59` (app/TIHarnessCommands: C59-2, C59-4, C59-9). The
// start is LIB-18's on TIAppPart's TIProductMachines (the built-in default:
// the Master Library module, no printer). The settings declaration and the
// ROM lookup are the product's (app/Settings.hpp, app/ProductRoms.hpp).

namespace cu59 {

/// The factory registered with calcu_harness_add_product (CMakeLists.txt).
std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct();

/// The rule of `lcd` for one position (LIB-11 AC-10, AC-16): a
/// digit, -, °, " or ' from the lit segments A–G (core/TISegments.hpp), a
/// blank for a position not driven, then "." for a lit decimal point, as
/// display() of 2.3.0's tools/headless/ti59.py (D-15: no hexadecimal digits).
std::string tiPositionText(const calcu::machine::SegmentPosition&);

} // namespace cu59

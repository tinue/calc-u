#pragma once
#include "harness/Product.hpp"
#include <memory>

// Calc-U-59 in the harness `calcu`: the
// library's app session on a TI machine with the product's app layer,
// TIAppPart, as the frontends host it (built in prepare() on the run's
// settings and folders; its library parts are the run's), the product
// commands under `c59` (app/TIHarnessCommands). The start is the library's
// machine control's on TIAppPart's TIProductMachines (the built-in default:
// the Master Library module, no printer). The settings declaration and the
// ROM lookup are the product's (app/Settings.hpp, app/ProductRoms.hpp).

namespace cu59 {

/// The factory registered with calcu_harness_add_product (CMakeLists.txt).
std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct();

/// The rule of `display-text` for one position: a
/// digit, -, °, " or ' from the lit segments A–G (core/TISegments.hpp), a
/// blank for a position not driven, then "." for a lit decimal point (no
/// hexadecimal digits).
std::string tiPositionText(const calcu::machine::SegmentPosition&);

} // namespace cu59

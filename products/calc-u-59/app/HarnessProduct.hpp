#pragma once
#include "harness/Product.hpp"
#include <memory>

// Calc-U-59 in the harness `calcu` (LIB-11 design §4; M-28): the library's
// app session on a TI machine through the machine interface, with the
// library's parts the entry makes itself until M-59 (LIB-6 §11.5): typing,
// paper, debugger, the presets on TIPresetRules and the bundle of the source
// tree's examples/, and the program memory commands (ProgramMemoryPart). The
// start is LIB-18's on TIProductMachines (the built-in default: the Master
// Library module, no printer; M-54). The product commands `ti59 partition`,
// `ti59 program`, `ti59 registers` (C59-2 AC-9); the rest of C59-9 in M-59.
// The settings declaration and the ROM lookup are the product's
// (app/Settings.hpp, app/ProductRoms.hpp).

namespace cu59 {

/// The factory registered with calcu_harness_add_product (CMakeLists.txt).
std::unique_ptr<calcu::harness::HarnessProduct> makeHarnessProduct();

/// The rule of `lcd` for one position (AC-10; AC-16 is checked in M-59): a
/// digit, -, °, " or ' from the lit segments A–G (core/TISegments.hpp), a
/// blank for a position not driven, then "." for a lit decimal point, as
/// display() of tools/headless/ti59.py.
std::string tiPositionText(const calcu::machine::SegmentPosition&);

} // namespace cu59

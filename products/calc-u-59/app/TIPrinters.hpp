#pragma once
#include "paper/PaperPart.hpp"
#include <vector>

// Calc-U-59's printers with a paper (LIB-8 design §11.3): the PC-100C with
// its description (core/PC100CPaper.hpp). The app session's PaperPart and
// the harness's take it.

namespace cu59 {

std::vector<calcu::paper::PrinterPaper> tiPrinterPapers();

} // namespace cu59

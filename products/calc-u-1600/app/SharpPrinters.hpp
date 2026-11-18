#pragma once
#include <vector>

#include "paper/PaperPart.hpp"

// Calc-U-1600's printers with a paper: the CE-150 and
// the CE-1600P with their descriptions (core/Connector/PlotterPaperOutput.hpp),
// in that order. The app session's PaperPart and the harness's take them;
// the CE-158's printer port stays a product kind.

namespace cu1600 {

std::vector<calcu::paper::PrinterPaper> sharpPrinterPapers();

} // namespace cu1600

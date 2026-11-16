#include "TIPrinters.hpp"
#include "../core/PC100CPaper.hpp"
#include "../core/TICatalogue.hpp"

namespace cu59 {

std::vector<calcu::paper::PrinterPaper> tiPrinterPapers() { return {{kPC100C, pc100cpaper::description()}}; }

} // namespace cu59

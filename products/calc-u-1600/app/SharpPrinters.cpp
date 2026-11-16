#include "app/SharpPrinters.hpp"

#include "Connector/PlotterPaperOutput.hpp"
#include "Machine/SharpCatalogue.hpp"

namespace shared {

std::vector<calcu::paper::PrinterPaper> sharpPrinterPapers() {
    return {
        calcu::paper::PrinterPaper{calcu::machine::PeripheralId{sharp::kCE150}, sharp::ce150PaperDescription()},
        calcu::paper::PrinterPaper{calcu::machine::PeripheralId{sharp::kCE1600P}, sharp::ce1600pPaperDescription()},
    };
}

} // namespace shared

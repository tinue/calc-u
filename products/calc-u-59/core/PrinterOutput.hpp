#pragma once
#include "TIPrinterTypes.hpp"
#include "machine/Capability.hpp"
#include <string>
#include <vector>

// The product kind `c59.printer-output` (C59-6 design §3.5): the paper the
// PC-100C prints, as text lines and code lines. An adapter for LIB-8's paper
// output kind, held in place until M-53 (LIB-8 59). Present only while a
// PC-100C is plugged in (C59-6 AC-1; LIB-1 EC-5); capability ID
// `printer/c59.printer-output`.

struct PrinterOutput : calcu::machine::Capability {
    static constexpr calcu::machine::KindId kKind{"c59.printer-output"};
    calcu::machine::KindId kind() const override { return kKind; }

    /// The text lines printed since the last call (a feed is an empty line).
    virtual std::vector<std::string> takeLines() = 0;
    /// The code lines, parallel to the text lines; a line still being
    /// printed comes when it ends.
    virtual std::vector<PrinterCodeLine> takeCodeLines() = 0;
    /// Before a detach (C59-6 EC-2): a line being printed ends where the head
    /// is, so that the caller can take its rows before the printer goes.
    virtual void endLineHere() = 0;
};

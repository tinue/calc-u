#pragma once
#include <cstdint>

// The printer connector's functional pin-out (C59-6 design §3.1; spec,
// Technical Requirements; docs/contracts/peripherals.md, fit table). The
// signals of the fit table: Busy (KR), KP, KN, Φ1, D0, D15, IRG, IDLE, EXT,
// Φ2, D12, I/A. The PC-100C sees only these; the TI machine maps the pads to
// the CPU's digit lines (D0 is digit 0 on the TI-59 and TI-58, digit 10 on
// the TI-58C), because the connector knows the host and the peripheral does
// not (peripherals.md I-1).

/// IRG: the six printer instruction words; KR[10:4] carries the data.
enum class PrinterWord : std::uint8_t { Out, Func, Clear, Step, Go, Feed };   // PRT.OUT … PRT.FEED

/// The digit strobes the PC-100C answers on with KP and KN.
enum class PrinterPad : std::uint8_t { D0, D12, D15 };

class PrinterPinOut {
public:
    virtual ~PrinterPinOut() = default;
    /// Φ1: once per instruction of the machine. An instruction in the ROM's
    /// IDLE state counts once, although it takes four times the ticks
    /// (C59-6 AC-4; the reading of peripherals.md P-2 in the spec).
    virtual void instructionCycle() = 0;
    /// IRG: a printer word with KR as data.
    virtual void word(PrinterWord, std::uint16_t kr) = 0;
    /// Busy, read by TST BUSY.
    virtual bool busy() const = 0;
    /// The key lines on a strobe: bit 2 KP, bit 0 KN.
    virtual std::uint8_t keyLines(PrinterPad) const = 0;
};

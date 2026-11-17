#pragma once
#include "MachineVariant.hpp"
#include <cstdint>

// ROM address ranges the machine keys off (C59-1 AC-18; was
// app/MachineModelInfo.hpp at 2.3.0). Core-internal: above the core, key
// readiness is Machine::isKeyReady(), the idle/key-detect range is
// idleAndKeyDetectRange() (TIAdapters.hpp) and the card wait is TICardReaderPlace::waitsForMedium (C59-7).

/// Inclusive ROM address range, with an optional second span.
struct PcRange {
    uint16_t lo, hi;
    uint16_t extraLo = 1, extraHi = 0;  ///< empty unless set
    constexpr bool contains(uint16_t pc) const {
        return (pc >= lo && pc <= hi) || (pc >= extraLo && pc <= extraHi);
    }
};

/// TI-58: every idle pass detours through the card veto 0x0459–0x045B (digit 10
/// open, D7-KR asserted → JNC 0641) — see rom/TI59-commented.asm at 0x063C.
inline constexpr uint16_t kCardVetoLo = 0x0459, kCardVetoHi = 0x045B;

/// TI-58C with a printer: every idle pass rescans below the printer line at D10
/// (0x0A25–0x0A2E; through 0x0A30 it confirms a key) — rom/TI58C-commented.asm at 0x0A25.
inline constexpr uint16_t kPrinterRescanLo = 0x0A25, kPrinterRescanHi = 0x0A2E, kPrinterConfirmHi = 0x0A30;

struct TIRomRanges {
    /// Steady-state keyboard scan loop: key readiness (and FREEZE ON START).
    /// With a printer it includes the confirmation scan, which the printer line
    /// (TI-59/58 D0, TI-58C D10) sends every pass through.
    PcRange scanLoop;
    /// Full keyboard scan/detect cycle — release-gate spin (0x0627) through key
    /// acceptance (0x0661 CLR.IDLE; TI-58C 0x065A). See rom/TI59-commented.asm
    /// (0x0620-0x0661) and ideas/KeypressLatch.md. Used wherever a held key or
    /// full-speed racing must not outrun real scanning/debouncing time.
    PcRange idleAndKeyDetect;
    /// The card-reader wait of Write / INV Write (0x16B2–0x16B7: R/S aborts,
    /// else poll the card switch) — rom/TI59-commented.asm at 0x16B2. The idle
    /// loop polls the card switch too, so isWaitingForCard() alone can't tell
    /// a program waiting for a card from an idle calculator. Empty without a reader.
    PcRange cardWait;
};

inline constexpr TIRomRanges romRanges(MachineVariant v) {
    switch (v) {
    case MachineVariant::TI58:
        return {{0x063C, 0x0660, kCardVetoLo, kCardVetoHi}, {0x0627, 0x0660, kCardVetoLo, kCardVetoHi}, {1, 0}};
    case MachineVariant::TI58C:
        return {{0x063D, 0x0659, kPrinterRescanLo, kPrinterRescanHi},
                {0x0627, 0x0659, kPrinterRescanLo, kPrinterConfirmHi}, {1, 0}};
    case MachineVariant::TI59:
    default:
        return {{0x063C, 0x0660}, {0x0627, 0x0660}, {0x16B2, 0x16B7}};
    }
}

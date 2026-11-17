#pragma once
#include "machine/Machine.hpp"
#include <cstdint>

// The run facts the timing commands of C59-9 read between two advances
// (design §3.2): through the product kind c59.calculator-state (the
// processor frame and the SCOM), no machine class (C59-1 AC-17).

namespace cu59 {

struct RunNote {
    int step = 0;                ///< SCOM[0] digits 7, 6, 5 (BCD register) × 8 + digit 4 (byte)
    std::uint8_t source = 0;     ///< SCOM[0] digit 3, the Prg Src flag
    bool printerBusy = false;    ///< the busy bit of the processor flags
    bool printWait = false;      ///< at one of the ROM's print-wait loops (TI-59 and TI-58)
    bool running = false;        ///< bit 0 of fB: a program runs
    bool displayOn = false;      ///< a digit or point driven in the last digit-counter revolution
    bool cpuIdle = false;        ///< no program, the idle flag, in the model's keyboard idle loop
    std::uint16_t pc = 0;
};

/// The note of the machine now; a machine that is not a TI gives the empty note.
RunNote noteRun(calcu::machine::Machine&);

/// Idle as LIB-6 AC-36 says: key readiness, no program, the printer not busy.
bool runIdle(calcu::machine::Machine&);

} // namespace cu59

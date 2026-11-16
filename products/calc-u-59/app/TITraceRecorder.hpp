#pragma once
#include "debugger/TraceSession.hpp"
#include "machine/Machine.hpp"
#include <functional>
#include <memory>

// Calc-U-59's trace recorder (LIB-10 design §12.6; spec AC-33, D-24): the
// processor frames of the c59.trace-records kind as the records of today's
// trace file, version 2 (app/TraceWriter): the header with `TI59` and the
// model, SESSION_START, one TRACE_EVENT per instruction, TRACE_GAP for
// frames the ring dropped, SESSION_END. No key and no card records. The
// button's file is CALCU59_TRACE.bin, CALCU58_TRACE.bin or
// CALCU58C_TRACE.bin after the model. tools/read_trace.py reads the files.

namespace cu59 {

// nullptr for a machine without the kind. `clock`: the seconds since the
// epoch written into SESSION_START; unset: the system clock.
std::unique_ptr<calcu::debugger::TraceRecorder> makeTITraceRecorder(calcu::machine::Machine&,
                                                                    std::function<std::uint64_t()> clock = {});

} // namespace cu59

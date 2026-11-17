#pragma once
#include <memory>

#include "debugger/TraceSession.hpp"
#include "machine/Machine.hpp"

// Calc-U-1600's trace recorder (LIB-10 design §10.7; spec AC-42): the records
// of the core's trace (the c1600.trace-records kind, D-18) into the library's
// trace file, in today's TRACE.bin format, version 3: header `PC15`, the
// session start and end, the instructions of the LH5801 or LH5803 and of the
// Z-80, gap records; both CPUs of a PC-1600 in one file. The button's file is
// TRACE.bin (AC-23). tools/read_trace.py reads the files unchanged.

namespace cu1600 {

// nullptr for a machine without the kind.
std::unique_ptr<calcu::debugger::TraceRecorder> makeSharpTraceRecorder(calcu::machine::Machine&);

} // namespace cu1600

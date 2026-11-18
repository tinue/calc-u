#pragma once
#include <memory>

#include "debugger/TraceSession.hpp"
#include "machine/Machine.hpp"

// Calc-U-1600's trace recorder: the records
// of the core's trace (the c1600.trace-records kind) into the library's
// trace file, in the TRACE.bin format, version 3: header `PC15`, the
// session start and end, the instructions of the LH5801 or LH5803 and of the
// Z-80, gap records; both CPUs of a PC-1600 in one file. The button's file is
// TRACE.bin. tools/read_trace.py reads the files unchanged.

namespace cu1600 {

// nullptr for a machine without the kind.
std::unique_ptr<calcu::debugger::TraceRecorder> makeSharpTraceRecorder(calcu::machine::Machine&);

} // namespace cu1600

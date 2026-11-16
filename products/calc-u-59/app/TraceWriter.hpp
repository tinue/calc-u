#pragma once
#include "../core/TraceTypes.hpp"
#include "MachineModelInfo.hpp"
#include <cstdint>
#include <string>
#include <vector>

// The records of Calc-U-59's binary trace file, format version 2
// (reference/DebugAPI.md; tools/read_trace.py reads it): a 16-byte file
// header, then records of [type u8][length u16 LE][payload]. Since M-53 an
// encoder into a byte buffer only (LIB-10 design §12.3): the library's
// trace session writes the file and keeps its size limit, and
// TITraceRecorder says what goes in. The key and card records are never
// written (LIB-10 AC-33).

namespace cu59 {

struct TraceWriter {
    /// The button's file name by model: CALCU59_TRACE.bin, CALCU58_TRACE.bin, CALCU58C_TRACE.bin.
    static std::string baseFileName(Model model);

    /// The file header (`TI59`, version 2, the model) and SESSION_START with
    /// `epochSeconds`.
    static void sessionStart(std::vector<std::uint8_t>& out, Model model, std::uint64_t epochSeconds);
    /// One TRACE_EVENT (kEventBytes).
    static void event(std::vector<std::uint8_t>& out, const CpuFrame& frame);
    /// TRACE_GAP: `count` frames lost to the ring (kGapBytes).
    static void gap(std::vector<std::uint8_t>& out, std::uint32_t count);
    /// SESSION_END with the number of events (kEndBytes).
    static void sessionEnd(std::vector<std::uint8_t>& out, std::uint32_t events);

    static constexpr std::size_t kEventBytes = 3 + 125;
    static constexpr std::size_t kGapBytes = 3 + 4;
    static constexpr std::size_t kEndBytes = 3 + 8;
};

} // namespace cu59

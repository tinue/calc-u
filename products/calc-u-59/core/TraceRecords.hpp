#pragma once
#include "TraceTypes.hpp"
#include "machine/Capability.hpp"
#include <cstdint>
#include <vector>

// The product kind `c59.trace-records` (LIB-10 design §12.3, §12.6, D-24):
// the processor frames of the TMC0501 for a trace, owner TMC0501, capability
// ID `TMC0501/c59.trace-records`. begin() turns the core's frame recording
// on (the ring starts afresh), take() hands over what ran since the last
// call, end() turns it off again. What the file holds is the app's
// (app/TITraceRecorder, ADR 0037); the library's trace session writes it.

class TraceRecords : public calcu::machine::Capability {
public:
    static constexpr calcu::machine::KindId kKind{"c59.trace-records"};
    calcu::machine::KindId kind() const override { return kKind; }

    virtual void begin() = 0;
    /// Up to `max` frames appended to `out`, oldest first; `lost`: frames the
    /// ring dropped before them (a gap). Returns the number appended.
    virtual std::uint32_t take(std::vector<CpuFrame>& out, std::uint32_t max, std::uint32_t& lost) = 0;
    virtual void end() = 0;
};

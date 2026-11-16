#pragma once
#include <cstdint>
#include <deque>
#include <vector>

#include "../TraceTypes.hpp"

// ── PC1500TraceFile ────────────────────────────────────────────────────────
//
// Core-side writer for the `TRACE.bin` binary instruction-trace format
// (renamed from `CALCU1500_TRACE.bin` in format v2, once the same file
// started carrying PC-1600 frames too). This is the *only* writer of the
// format -- every trace of the app (the debugger's trace, the harness's
// `trace` command) goes through this same class, so there is nothing else
// that needs to stay in sync with it except the reader: `tools/read_trace.py`'s constant block and frame
// parsing MUST move together with this file's magic, version, the
// record-type enum, and both TRACE_EVENT field orders (25-byte
// LH5801-shaped -- CpuFrame, used by both the PC-1500/1500A's own CPU and
// the PC-1600's LH5803 co-processor -- and 29-byte Z80-shaped --
// Z80CpuFrame, the PC-1600's SC7852).
//
// Both CPUs' frames land in ONE file (not two), tagged by `cpuId`
// (CpuFrame/Z80CpuFrame's own field, Core/TraceTypes.hpp) -- safe because
// PC1600BusArbiter guarantees only one CPU ever executes at a time (no
// real concurrency to race), and each CPU's own trace ring has an
// independent internal mutex, so draining both every tick can't deadlock
// against anything.
//
// It keeps the bytes of whole records until take() hands them on (LIB-10
// §10.7: the c1600.trace-records kind); the library's trace session writes,
// limits and ends the file.
//
// Not thread-safe. `PC1500Machine` only ever touches its instance from
// whichever thread is stepping the CPU.
class PC1500TraceFile {
public:
    /// The 16-byte file header and a SESSION_START record wait in the
    /// buffer.
    PC1500TraceFile();

    PC1500TraceFile(const PC1500TraceFile&) = delete;
    PC1500TraceFile& operator=(const PC1500TraceFile&) = delete;

    /// Writes one TRACE_EVENT record (LH5801-shaped -- the PC-1500/1500A's
    /// own CPU, or the PC-1600's LH5803 co-processor). Field groups not
    /// populated by the active TRACE_* flags are simply written as their
    /// zero-initialised `CpuFrame` values.
    void writeFrame(const CpuFrame& f);
    /// Writes one TRACE_EVENT_Z80 record (the PC-1600's SC7852).
    void writeFrame(const Z80CpuFrame& f);

    /// Writes a TRACE_GAP record: `lostFrames` instructions were
    /// overwritten in the ring before they could be drained.
    void writeGap(uint32_t lostFrames);

    /// Writes a SESSION_END record (with the running event count).
    /// Idempotent -- a second call is a no-op.
    void finish();

    /// Appends to `out` the bytes of whole records waiting, as many as fit
    /// in `budget`. False if records are left (the budget was not enough).
    bool take(std::vector<uint8_t>& out, uint64_t budget);

private:
    bool m_finished = false;
    uint32_t m_eventCount = 0;
    std::vector<uint8_t> m_buffer;   // buffer mode: what take() has not handed on
    std::deque<uint64_t> m_ends;     // the end of each record in m_buffer, from its start

    void put(const uint8_t* data, size_t n);
    void startSession();

    void writeRecord(uint8_t type, const uint8_t* payload, uint16_t len);
};

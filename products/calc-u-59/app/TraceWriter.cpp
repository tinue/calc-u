#include "TraceWriter.hpp"

namespace cu59 {

namespace {

enum : uint8_t {
    kSessionStart = 0x01,
    kTraceEvent   = 0x02,
    kSessionEnd   = 0x03,
    kUserEvent    = 0x04,  // key and card records: never written
    kTraceGap     = 0x05,  // payload: u32 count of lost frames
};

constexpr uint32_t kMagic = 0x54493539;  // 'TI59' LE
// v1 (released in v1.0.0) is stable; v2+ can evolve without backwards-compat guarantees.
constexpr uint16_t kVersion = 2;
constexpr size_t kFramePayload = 125;

struct Buf {
    std::vector<uint8_t>& b;
    void u8(uint8_t v) { b.push_back(v); }
    void u16(uint16_t v) { u8(static_cast<uint8_t>(v & 0xFF)); u8(static_cast<uint8_t>(v >> 8)); }
    void u32(uint32_t v) { u16(static_cast<uint16_t>(v & 0xFFFF)); u16(static_cast<uint16_t>(v >> 16)); }
    void u64(uint64_t v) { u32(static_cast<uint32_t>(v & 0xFFFFFFFF)); u32(static_cast<uint32_t>(v >> 32)); }
    void bytes(const uint8_t* p, size_t n) { b.insert(b.end(), p, p + n); }
    void record(uint8_t type, uint16_t length) { u8(type); u16(length); }
};

} // namespace

std::string TraceWriter::baseFileName(Model model) {
    switch (model) {
    case Model::TI59:  return "CALCU59_TRACE.bin";
    case Model::TI58:  return "CALCU58_TRACE.bin";
    case Model::TI58C: return "CALCU58C_TRACE.bin";
    }
    return "CALCU59_TRACE.bin";
}

void TraceWriter::sessionStart(std::vector<uint8_t>& out, Model model, uint64_t epochSeconds) {
    Buf b{out};
    b.u32(kMagic);   // 16-byte header
    b.u16(kVersion);
    b.u16(static_cast<uint16_t>(model));   // v2: 0 = TI-59, 1 = TI-58, 2 = TI-58C
    b.u64(0);                              // reserved
    b.record(kSessionStart, 9);
    b.u64(epochSeconds);
    b.u8(static_cast<uint8_t>(model));
}

void TraceWriter::event(std::vector<uint8_t>& out, const CpuFrame& f) {
    Buf d{out};
    d.record(kTraceEvent, kFramePayload);
    // Every frame is written, no deduplication, to keep the exact execution trace.
    d.u32(0);  // suppressed count, always 0
    d.u32(f.seqno);
    d.u16(f.pc);
    d.u16(f.opcode);
    d.u16(f.fA);
    d.u16(f.fB);
    d.u16(f.KR);
    d.u16(f.SR);
    d.u16(f.EXT);
    d.u16(f.PREG);
    d.u16(f.flags);
    d.u16(f.m_libAddr);
    d.u8(f.R5);
    d.u8(f.digit);
    d.u8(f.RAM_ADDR);
    d.u8(f.RAM_OP);
    d.u8(f.REG_ADDR);
    d.u8(f.m_libAddrReadPos);
    d.u8(f.cycleWeight);
    d.u8(f.displayOn);
    d.u8(f.maxDigitDecay);
    // Registers A–E: one nibble per byte, index 0 = least significant digit.
    d.bytes(f.A, 16);
    d.bytes(f.B, 16);
    d.bytes(f.C, 16);
    d.bytes(f.D, 16);
    d.bytes(f.E, 16);
    // Sout nibble-packed: low nibble = Sout[2i], high nibble = Sout[2i+1].
    for (int i = 0; i < 8; ++i)
        d.u8(static_cast<uint8_t>((f.Sout[2 * i] & 0x0F) | ((f.Sout[2 * i + 1] & 0x0F) << 4)));
}

void TraceWriter::gap(std::vector<uint8_t>& out, uint32_t count) {
    Buf b{out};
    b.record(kTraceGap, 4);
    b.u32(count);
}

void TraceWriter::sessionEnd(std::vector<uint8_t>& out, uint32_t events) {
    Buf b{out};
    b.record(kSessionEnd, 8);
    b.u32(events);
    b.u32(0);  // suppressed-event total: always 0 (no deduplication), kept for the format
}

} // namespace cu59

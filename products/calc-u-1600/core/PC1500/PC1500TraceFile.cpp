#include "PC1500TraceFile.hpp"

#include <ctime>

namespace {

// ── Format constants ─────────────────────────────────────────────────────
// MUST match tools/read_trace.py's constant block.
constexpr uint32_t kMagic   = 0x50433135;  // 'PC15'
constexpr uint16_t kVersion = 3;  // v3: registers are pre-execution (layout as v2: 25-byte TRACE_EVENT, TRACE_EVENT_Z80)
constexpr int      kHeaderSize = 16;

enum RecType : uint8_t {
    kRecSessionStart  = 0x01,
    kRecTraceEvent    = 0x02, // CpuFrame (LH5801-shaped): PC-1500/1500A's own CPU, or PC-1600's LH5803
    kRecSessionEnd    = 0x03,
    kRecTraceGap      = 0x05,
    kRecTraceEventZ80 = 0x04, // Z80CpuFrame-shaped: PC-1600's SC7852
};

// Little-endian append helpers into a fixed-size local byte buffer.
struct ByteSink {
    uint8_t* p;
    void u8(uint8_t v) { *p++ = v; }
    void u16(uint16_t v) {
        *p++ = uint8_t(v & 0xFF);
        *p++ = uint8_t(v >> 8);
    }
    void u32(uint32_t v) {
        *p++ = uint8_t(v & 0xFF);
        *p++ = uint8_t((v >> 8) & 0xFF);
        *p++ = uint8_t((v >> 16) & 0xFF);
        *p++ = uint8_t((v >> 24) & 0xFF);
    }
    void u64(uint64_t v) {
        u32(uint32_t(v & 0xFFFFFFFFu));
        u32(uint32_t(v >> 32));
    }
};

} // namespace

PC1500TraceFile::PC1500TraceFile() { startSession(); }

void PC1500TraceFile::startSession() {
    uint8_t header[kHeaderSize];
    ByteSink h{header};
    h.u32(kMagic);
    h.u16(kVersion);
    h.u16(0);   // reserved
    h.u64(0);   // reserved for future use
    put(header, sizeof(header));
    m_ends.push_back(m_buffer.size());

    uint8_t payload[8];
    ByteSink s{payload};
    s.u64(static_cast<uint64_t>(std::time(nullptr)));
    writeRecord(kRecSessionStart, payload, sizeof(payload));
}

void PC1500TraceFile::put(const uint8_t* data, size_t n) { m_buffer.insert(m_buffer.end(), data, data + n); }

bool PC1500TraceFile::take(std::vector<uint8_t>& out, uint64_t budget) {
    uint64_t upTo = 0;
    while (!m_ends.empty() && m_ends.front() <= budget) {
        upTo = m_ends.front();
        m_ends.pop_front();
    }
    out.insert(out.end(), m_buffer.begin(), m_buffer.begin() + static_cast<std::ptrdiff_t>(upTo));
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<std::ptrdiff_t>(upTo));
    for (uint64_t& e : m_ends) e -= upTo;
    return m_ends.empty();
}

void PC1500TraceFile::writeRecord(uint8_t type, const uint8_t* payload, uint16_t len) {
    uint8_t recHeader[3];
    ByteSink h{recHeader};
    h.u8(type);
    h.u16(len);
    put(recHeader, sizeof(recHeader));
    if (len) put(payload, len);
    m_ends.push_back(m_buffer.size());
}

void PC1500TraceFile::writeFrame(const CpuFrame& f) {
    // 25-byte TRACE_EVENT payload -- field order MUST match read_trace.py's
    // `<IHHB BBBBBBB HB BBB H B` unpack (trailing B = cpuId, v2).
    uint8_t payload[25];
    ByteSink s{payload};
    s.u32(f.seqno);
    s.u16(f.pc);
    s.u16(f.opcode);
    s.u8(f.cycles);
    s.u8(f.a);
    s.u8(f.xl);
    s.u8(f.xh);
    s.u8(f.yl);
    s.u8(f.yh);
    s.u8(f.ul);
    s.u8(f.uh);
    s.u16(f.s);
    s.u8(f.t);
    s.u8(f.pu);
    s.u8(f.pv);
    s.u8(f.disp);
    s.u16(f.tm);
    s.u8(f.cpuId);
    writeRecord(kRecTraceEvent, payload, sizeof(payload));
    m_eventCount++;
}

void PC1500TraceFile::writeFrame(const Z80CpuFrame& f) {
    // 29-byte TRACE_EVENT_Z80 payload -- field order MUST match
    // read_trace.py's `<IHHBB HHHHHHH BBBBB` unpack.
    uint8_t payload[29];
    ByteSink s{payload};
    s.u32(f.seqno);
    s.u16(f.pc);
    s.u16(f.opcode);
    s.u8(f.cycles);
    s.u8(f.cpuId);
    s.u16(f.af);
    s.u16(f.bc);
    s.u16(f.de);
    s.u16(f.hl);
    s.u16(f.ix);
    s.u16(f.iy);
    s.u16(f.sp);
    s.u8(f.i);
    s.u8(f.r);
    s.u8(f.iff1 ? 1 : 0);
    s.u8(f.iff2 ? 1 : 0);
    s.u8(f.im);
    writeRecord(kRecTraceEventZ80, payload, sizeof(payload));
    m_eventCount++;
}

void PC1500TraceFile::writeGap(uint32_t lostFrames) {
    uint8_t payload[4];
    ByteSink s{payload};
    s.u32(lostFrames);
    writeRecord(kRecTraceGap, payload, sizeof(payload));
}

void PC1500TraceFile::finish() {
    if (m_finished) return;
    m_finished = true;
    uint8_t payload[4];
    ByteSink s{payload};
    s.u32(m_eventCount);
    writeRecord(kRecSessionEnd, payload, sizeof(payload));
}

#pragma once
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

// A minimal reader of the TRACE.bin format, version 3, for
// the tests: what records a file holds. Constants MUST match
// core/PC1500/PC1500TraceFile.cpp (and tools/read_trace.py).

namespace tracetest {

struct TraceSummary {
    bool headerOk = false;
    int firstRecordType = -1;
    int lastRecordType = -1;
    int sessionStartCount = 0;
    int traceEventCount = 0;
    int sessionEndCount = 0;
    int gapCount = 0;
    int z80EventCount = 0;          // 0x04 TRACE_EVENT_Z80 (the PC-1600's SC7852)
    bool truncated = false;
    bool allEventPayloads25 = true; // v2: 25-byte TRACE_EVENT (added cpuId)
    bool allZ80Payloads29 = true;   // 29-byte TRACE_EVENT_Z80
};

inline uint32_t le32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline uint16_t le16(const uint8_t* p) { return uint16_t(uint16_t(p[0]) | (uint16_t(p[1]) << 8)); }

inline TraceSummary readTrace(const std::string& path) {
    TraceSummary s;
    std::ifstream in(path, std::ios::binary);
    if (!in) return s;
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (buf.size() < 16) { s.truncated = true; return s; }
    s.headerOk = (le32(buf.data()) == 0x50433135u) && (le16(buf.data() + 4) == 3);

    size_t i = 16;
    while (i + 3 <= buf.size()) {
        uint8_t type = buf[i];
        uint16_t len = le16(buf.data() + i + 1);
        if (i + 3 + len > buf.size()) { s.truncated = true; break; }
        if (s.firstRecordType < 0) s.firstRecordType = type;
        s.lastRecordType = type;
        switch (type) {
            case 0x01: s.sessionStartCount++; break;
            case 0x02: s.traceEventCount++; if (len != 25) s.allEventPayloads25 = false; break;
            case 0x03: s.sessionEndCount++; break;
            case 0x04: s.z80EventCount++; if (len != 29) s.allZ80Payloads29 = false; break;
            case 0x05: s.gapCount++; break;
            default: break;
        }
        i += 3 + len;
    }
    return s;
}


} // namespace tracetest

// One-time verification (dev/loader-matrix/README.md), PC-1600 part: loads
// one program twice -- through the ROM over COM1: (`LOAD`/`BLOAD "COM1:"`)
// or over the CE-158 (`CLOAD`/`CLOAD M`, MODE 1 only), fed the bytes `sde
// put` sends, and through the emulator's own loader (a preset's `program:
// file:`) -- and compares the memory afterwards. The CE-158 is attached in
// every run.
//
//   pc1600_loader_matrix --probe --mode <0|1> [--slot1 <card>] [--slot2 <card>]
//   pc1600_loader_matrix --mode <0|1> [--slot1 <card>] [--slot2 <card>]
//                        --transport <com1|ce158> --kind <basic|ml>
//                        --stream <sde file> --loader-file <file> [--expect-refuse]
//
// --stream is the `sde convert` output -- what `sde put` writes: the header
// (16 bytes PC-1600, 27 bytes CE-158) at full speed, a 300 ms pause, then
// the payload a byte at a time with 1 ms between bytes.
//
// The machine is built through the machine interface (C1600-1 design §7;
// LoaderMatrixCommon.hpp); the serial peer works on the serial port
// capability of COM1: (the component rs232c) or of the CE-158.
//
// Build: dev/loader-matrix/build_pc1600.sh. Run from the repo root.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "LoaderMatrixCommon.hpp"

namespace {

using matrix::Machine;

constexpr const char* kScratchDir = "headless/loader-matrix";

// Work-area bytes that hold where the program is and how far it goes. They
// must match after both loads (PC-1600-Work-Area-Map.md).
struct Span { uint16_t from, to; const char* name; };
const Span kPointers[] = {
    {0xF016, 0xF023, "S1MTb/S1MBb/S2MTb/S2MBb descriptors"},
    {0xF02A, 0xF02C, "S0MTb, BASPRG_ST/END banks"},
    {0xF1C1, 0xF1C1, "CURRENT_TOP bank"},
    {0xF1D5, 0xF1DA, "TITLE, ADTBL"},
    {0xF865, 0xF86A, "BASPRG_ST/END/EDT"},
    {0xF899, 0xF89A, "VARIABLE_PTR"},
    {0xF89D, 0xF89E, "F89D / CURRENT_TOP"},
    {0xFE3C, 0xFE41, "PRGADR"},
};

struct Config {
    int mode = 0;
    std::string slot1, slot2;
    bool com1 = true;
    bool ml = false;
};

struct Snapshot {
    std::vector<uint8_t> ram;          // Z-80 $C000-$FFFF (LH5803 $4000-$7FFF)
    std::vector<uint8_t> slot1, slot2; // the cards' backing stores
};

Snapshot snapshot(Machine& m) {
    Snapshot s;
    s.ram.resize(0x4000);
    for (uint32_t a = 0; a < 0x4000; ++a) s.ram[a] = cli::peek(m, uint16_t(0xC000 + a));
    const sharp::InspectorViews* inspector = sharp::firstOf<sharp::InspectorViews>(m);
    int banks = -1;
    s.slot1 = inspector->slotImage(1, &banks);
    s.slot2 = inspector->slotImage(2, &banks);
    return s;
}

std::string absPath(const std::string& p) {
    char buf[4096];
    return (!p.empty() && p[0] != '/' && realpath(p.c_str(), buf)) ? std::string(buf) : p;
}

// Builds and applies the preset: CE-158, the slot cards, MODE, PRO mode,
// NEW0, then `extraKeys`, then `program: file:` if `loaderFile` is set.
// `com1`: the RS-232C gets its peer before the preset runs (the CE-158's
// port exists once the preset has plugged it in).
matrix::Loaded applyPreset(const Config& c, bool com1, const std::vector<std::string>& extraKeys,
                                     const std::string& loaderFile, PresetLoadResult* res) {
    std::string yaml = "format-version: 2\nmodel: PC-1600\ninterface: CE-158\n";
    if (!c.slot1.empty()) yaml += "slot-1: { name: " + c.slot1 + " }\n";
    if (!c.slot2.empty()) yaml += "slot-2: { name: " + c.slot2 + " }\n";
    yaml += "keys:\n";
    if (c.mode == 1) yaml += "  - type: MODE 1\n";
    yaml += "  - key: mode\n  - type: NEW0\n";
    for (const std::string& k : extraKeys) yaml += "  - type: " + k + "\n";
    if (!loaderFile.empty()) yaml += "program:\n  file: " + absPath(loaderFile) + "\n";
    const std::string path = std::string(kScratchDir) + "/run.pc1600";
    std::ofstream(path) << yaml;
    return matrix::applyPreset(path, com1, res);
}

std::vector<std::string> serialSetup(const Config& c) {
    if (c.com1) return {"SETCOM \"COM1:\",9600,8,N,1,N,N", "RCVSTAT \"COM1:\",28"};
    return {"SETCOM 1200", "SETDEV CI"};
}

struct Run {
    bool ok = false;
    std::string note;
    Snapshot mem;
};

Run runRom(const Config& c, const std::vector<uint8_t>& stream) {
    Run r;
    matrix::SdeFeeder link;
    link.rx = stream;
    link.headerLen = c.com1 ? 16 : 27;
    const int baud = c.com1 ? 9600 : 1200;
    link.pauseChars = long(0.3 * baud / 10 + 0.5);
    link.gapChars = c.com1 ? 1 : 0; // 1 ms is about one character time at 9600, < one at 1200
    std::vector<std::string> keys = serialSetup(c);
    if (c.com1) keys.push_back(c.ml ? "BLOAD \"COM1:\"" : "LOAD \"COM1:\"");
    else keys.push_back(c.ml ? "CLOAD M" : "CLOAD");
    PresetLoadResult res;
    auto m = applyPreset(c, c.com1, keys, "", &res);
    if (!m || !res.ok) {
        r.note = "preset failed: " + res.error;
        return r;
    }
    calcu::peripherals::SerialPortQueues* port =
        c.com1 ? matrix::serialPort(*m, "rs232c", nullptr) : matrix::serialPort(*m, nullptr, sharp::kCE158);
    link.armed = true;
    const uint64_t hz = m->descriptor().tickRate;
    const uint64_t charTicks = hz * 10 / static_cast<uint64_t>(baud);   // the peer acts once per character time
    const double secs = 30 + stream.size() * (10.0 / baud) * (1 + link.gapChars) * 1.5;
    uint64_t budget = uint64_t(secs * static_cast<double>(hz));
    while (port && !link.done(port) && budget > charTicks) {
        budget -= std::min(budget, m->advance(charTicks));
        link.pump(*port);
    }
    m->advance(hz * 2);
    const std::string screen = matrix::lcd(*m);
    r.ok = link.done(port) && screen.find("ERROR") == std::string::npos;
    r.note = (link.done(port) ? "" : "stream not consumed (" + std::to_string(link.pos) + "/" +
                                         std::to_string(stream.size()) + "); ") + "lcd=[" + screen + "]";
    r.mem = snapshot(*m);
    return r;
}

Run runLoader(const Config& c, const std::string& loaderFile) {
    Run r;
    PresetLoadResult res;
    auto m = applyPreset(c, false, serialSetup(c), loaderFile, &res);
    if (!m) { r.note = res.error; return r; }
    m->advance(m->descriptor().tickRate / 2);
    r.ok = res.ok;
    r.note = (res.ok ? "" : "error: " + res.error + "; ") + "lcd=[" + matrix::lcd(*m) + "]";
    r.mem = snapshot(*m);
    return r;
}

// Differing byte ranges of a/b over [from, to), named by `base` + offset.
std::string ranges(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, uint32_t from, uint32_t to,
                   uint32_t base, size_t* count) {
    std::string out;
    *count = 0;
    if (a.size() != b.size()) { *count = 1; return "size differs"; }
    to = std::min<uint32_t>(to, uint32_t(a.size()));
    for (uint32_t x = from; x < to;) {
        if (a[x] == b[x]) { ++x; continue; }
        uint32_t y = x;
        while (y < to && a[y] != b[y]) ++y;
        *count += y - x;
        char buf[96];
        std::snprintf(buf, sizeof buf, "%s&%04X-&%04X", out.empty() ? "" : ",", base + x, base + y - 1);
        out += buf;
        if (y - x <= 2) {
            for (uint32_t z = x; z < y; ++z) {
                std::snprintf(buf, sizeof buf, "%s%02X/%02X", z == x ? "(" : " ", a[z], b[z]);
                out += buf;
            }
            out += ")";
        }
        x = y;
    }
    return out;
}

uint16_t be16(const std::vector<uint8_t>& ram, uint16_t z80) {
    return uint16_t(ram[z80 - 0xC000] << 8 | ram[z80 - 0xC000 + 1]);
}

int probe(const Config& c) {
    PresetLoadResult res;
    auto m = applyPreset(c, false, {"MEM"}, "", &res);
    if (!m || !res.ok) {
        std::fprintf(stderr, "preset failed: %s\n", res.error.c_str());
        return 2;
    }
    const Snapshot s = snapshot(*m);
    std::string screen = matrix::lcd(*m);
    long mem = -1;
    for (size_t i = screen.find("MEM"); i != std::string::npos && i < screen.size(); ++i) {
        if (isdigit(static_cast<unsigned char>(screen[i]))) { mem = std::strtol(screen.c_str() + i, nullptr, 10); break; }
    }
    // BASPRG_ST is kept LH5803-side; Z-80 = +$8000.
    std::printf("BMODE=%02X BASPRG_ST=%04X BANK=%02X VARIABLE_PTR=%04X MEM=%ld ADTBL=%02X,%02X,%02X,%02X,%02X\n",
                s.ram[0xF1BC - 0xC000], be16(s.ram, 0xF865), s.ram[0xF02B - 0xC000], be16(s.ram, 0xF899), mem,
                s.ram[0xF1D6 - 0xC000], s.ram[0xF1D7 - 0xC000], s.ram[0xF1D8 - 0xC000], s.ram[0xF1D9 - 0xC000],
                s.ram[0xF1DA - 0xC000]);
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Config c;
    std::string transport = "com1", kind, streamPath, loaderFile;
    bool doProbe = false, expectRefuse = false;
    for (int i = 1; i < argc; ++i) {
        auto val = [&](std::string* dst) { if (i + 1 < argc) *dst = argv[++i]; };
        std::string mode;
        if (!std::strcmp(argv[i], "--probe")) doProbe = true;
        else if (!std::strcmp(argv[i], "--expect-refuse")) expectRefuse = true;
        else if (!std::strcmp(argv[i], "--mode")) { val(&mode); c.mode = std::atoi(mode.c_str()); }
        else if (!std::strcmp(argv[i], "--slot1")) val(&c.slot1);
        else if (!std::strcmp(argv[i], "--slot2")) val(&c.slot2);
        else if (!std::strcmp(argv[i], "--transport")) val(&transport);
        else if (!std::strcmp(argv[i], "--kind")) val(&kind);
        else if (!std::strcmp(argv[i], "--stream")) val(&streamPath);
        else if (!std::strcmp(argv[i], "--loader-file")) val(&loaderFile);
        else { std::fprintf(stderr, "unknown argument %s\n", argv[i]); return 2; }
    }
    if (c.slot1 == "none") c.slot1.clear();
    if (c.slot2 == "none") c.slot2.clear();
    c.com1 = transport == "com1";
    c.ml = kind == "ml";
    if (doProbe) return probe(c);

    std::vector<uint8_t> stream;
    const size_t headerLen = c.com1 ? 16 : 27;
    if (!matrix::readFile(streamPath, &stream) || stream.size() <= headerLen) {
        std::fprintf(stderr, "cannot read stream %s\n", streamPath.c_str());
        return 2;
    }

    Run a = runRom(c, stream);
    Run b = runLoader(c, loaderFile);
    std::printf("ROM:    %s %s\n", a.ok ? "ok" : "REFUSED/FAILED", a.note.c_str());
    std::printf("LOADER: %s %s\n", b.ok ? "ok" : "REFUSED/FAILED", b.note.c_str());
    if (expectRefuse) {
        const bool pass = !a.ok && !b.ok;
        std::printf("RESULT %s (expect refuse)\n", pass ? "PASS" : "FAIL");
        return pass ? 0 : 1;
    }
    if (a.mem.ram.empty() || b.mem.ram.empty()) {
        std::printf("RESULT FAIL (no memory snapshot)\n");
        return 1;
    }

    size_t nUser = 0, nS1 = 0, nS2 = 0, nPtr = 0, nWork = 0;
    const std::string dUser = ranges(a.mem.ram, b.mem.ram, 0x0000, 0x3000, 0xC000, &nUser);
    const std::string dS1 = ranges(a.mem.slot1, b.mem.slot1, 0, uint32_t(a.mem.slot1.size()), 0, &nS1);
    const std::string dS2 = ranges(a.mem.slot2, b.mem.slot2, 0, uint32_t(a.mem.slot2.size()), 0, &nS2);
    std::string dPtr;
    for (const Span& s : kPointers) {
        size_t n = 0;
        const std::string d = ranges(a.mem.ram, b.mem.ram, s.from - 0xC000, s.to - 0xC000 + 1, 0xC000, &n);
        if (n) { nPtr += n; dPtr += std::string(dPtr.empty() ? "" : ", ") + s.name + " " + d; }
    }
    const std::string dWork = ranges(a.mem.ram, b.mem.ram, 0x3000, 0x4000, 0xC000, &nWork);

    std::printf("BASPRG_ST rom=%04X loader=%04X  BASPRG_END rom=%04X loader=%04X\n", be16(a.mem.ram, 0xF865),
                be16(b.mem.ram, 0xF865), be16(a.mem.ram, 0xF867), be16(b.mem.ram, 0xF867));
    std::printf("internal RAM &C000-&EFFF diff: %zu bytes %s\n", nUser, dUser.c_str());
    std::printf("slot 1 image diff: %zu bytes %s\n", nS1, dS1.c_str());
    std::printf("slot 2 image diff: %zu bytes %s\n", nS2, dS2.c_str());
    std::printf("pointers diff: %zu bytes %s\n", nPtr, dPtr.c_str());
    std::printf("work area &F000-&FFFF diff (info): %zu bytes %s\n", nWork, dWork.c_str());

    const bool pass = a.ok && b.ok && nUser == 0 && nS1 == 0 && nS2 == 0 && nPtr == 0;
    std::printf("RESULT %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}

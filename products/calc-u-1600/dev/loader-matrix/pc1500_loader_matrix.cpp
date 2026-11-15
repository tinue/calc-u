// One-time verification (dev/loader-matrix/README.md): loads one program into
// a PC-1500/1500A twice -- through the ROM (CLOAD / CLOAD M over the CE-158,
// fed the bytes `sde put` sends) and through the emulator's own loader (a
// preset's `program: file:`) -- and compares the memory afterwards. The
// CE-158 is attached in both runs.
//
//   pc1500_loader_matrix --probe --model <PC-1500|PC-1500A> [--card <name>]
//   pc1500_loader_matrix --model <m> [--card <name>] --kind <basic|ml>
//                        --stream <sde file> --loader-file <file> [--expect-refuse]
//
// --stream is the `sde convert` output (CE-158 header + payload), which is
// what `sde put` writes to the wire: header first, a 300 ms pause, then the
// payload. --loader-file is what our loader gets (`.bas` or `.ce158.bin`).
// --expect-refuse: the program doesn't fit; both paths must refuse it.
//
// The machines are built through the machine interface (C1600-1 design §7;
// LoaderMatrixCommon.hpp); the CE-158's serial peer works on its serial
// port capability.
//
// Build: dev/loader-matrix/build.sh. Run from the repo root.

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

constexpr size_t kHeaderLen = 27;                 // CE-158 header
constexpr const char* kScratchDir = "headless/loader-matrix";
constexpr uint16_t kRamSt = 0x7863, kRamEnd = 0x7864, kBasPrgSt = 0x7865, kBasPrgEnd = 0x7867;

uint8_t peek(Machine& m, uint16_t a) { return cli::peek(m, a); }

uint16_t peek16(Machine& m, uint16_t a) { // big-endian, as the ROM keeps them
    return uint16_t(peek(m, a) << 8 | peek(m, uint16_t(a + 1)));
}

std::vector<uint8_t> snapshot(Machine& m) {
    std::vector<uint8_t> mem(0x8000);
    for (uint32_t a = 0; a < mem.size(); ++a) mem[a] = peek(m, uint16_t(a));
    return mem;
}

struct Run {
    bool ok = false;          // the load succeeded
    std::string note;         // error / LCD text
    std::vector<uint8_t> mem;
};

// Builds and applies a preset. `extraKeys` are typed after NEW0; `loaderFile`
// non-empty adds `program: file:`.
std::unique_ptr<Machine> applyPreset(const std::string& model, const std::string& card,
                                     const std::vector<std::string>& extraKeys, const std::string& loaderFile,
                                     PresetLoadResult* res) {
    std::string yaml = "format-version: 1\nmodel: " + model + "\ninterface: CE-158\n";
    if (!card.empty()) yaml += "slot-1: " + card + "\n";
    yaml += "keys:\n  - key: cl\n  - type: NEW0\n";
    for (const std::string& k : extraKeys) yaml += "  - type: " + k + "\n";
    if (!loaderFile.empty()) yaml += "program:\n  file: " + loaderFile + "\n";
    const std::string path = std::string(kScratchDir) + "/run.pc1500";
    std::ofstream(path) << yaml;
    return matrix::applyPreset(path, false, res);
}

const std::vector<std::string> kSerialSetup = {"SETCOM 1200", "SETDEV CI"};

Run runRom(const std::string& model, const std::string& card, bool ml, const std::vector<uint8_t>& stream) {
    Run r;
    matrix::SdeFeeder link;
    link.rx = stream;
    link.headerLen = kHeaderLen;
    PresetLoadResult res;
    std::vector<std::string> keys = kSerialSetup;
    keys.push_back(ml ? "CLOAD M" : "CLOAD");
    auto m = applyPreset(model, card, keys, "", &res);
    if (!m || !res.ok) {
        r.note = "preset failed: " + res.error;
        return r;
    }
    calcu::peripherals::SerialPortQueues* port = matrix::serialPort(*m, nullptr, sharp::kCE158);
    // 300 ms at 1200 baud, 10 bits a character.
    link.pauseChars = long(0.3 * 1200 / 10 + 0.5);
    link.armed = true;
    const uint64_t hz = m->descriptor().tickRate;
    const uint64_t charTicks = hz * 10 / 1200;   // the peer acts once per character time
    uint64_t budget = hz * (60 + stream.size() / 100); // > stream time at 1200 baud
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

Run runLoader(const std::string& model, const std::string& card, const std::string& loaderFile) {
    Run r;
    PresetLoadResult res;
    std::string abs = loaderFile;
    if (!abs.empty() && abs[0] != '/') {
        char buf[4096];
        if (realpath(loaderFile.c_str(), buf)) abs = buf;
    }
    auto m = applyPreset(model, card, kSerialSetup, abs, &res);
    if (!m) {
        r.note = res.error;
        return r;
    }
    m->advance(m->descriptor().tickRate / 2);
    r.ok = res.ok;
    r.note = (res.ok ? "" : "error: " + res.error + "; ") + "lcd=[" + matrix::lcd(*m) + "]";
    r.mem = snapshot(*m);
    return r;
}

std::string ranges(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, uint32_t from, uint32_t to,
                   size_t* count) {
    std::string out;
    *count = 0;
    for (uint32_t x = from; x < to;) {
        if (a[x] == b[x]) { ++x; continue; }
        uint32_t y = x;
        while (y < to && a[y] != b[y]) ++y;
        *count += y - x;
        char buf[96];
        std::snprintf(buf, sizeof buf, "%s&%04X-&%04X", out.empty() ? "" : ",", x, y - 1);
        out += buf;
        if (y - x <= 2) { // short range: show rom/loader values
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

int probe(const std::string& model, const std::string& card) {
    PresetLoadResult res;
    auto m = applyPreset(model, card, {}, "", &res);
    if (!m || !res.ok) {
        std::fprintf(stderr, "preset failed: %s\n", res.error.c_str());
        return 2;
    }
    const uint16_t st = peek16(*m, kBasPrgSt);
    const uint32_t ramEnd = uint32_t(peek(*m, kRamEnd)) << 8;
    // MEM = RAM_END*256 - (BASPRG_END + 1) - ... ; print the plain span.
    std::printf("BASPRG_ST=%04X RAM_ST=%02X00 RAM_END=%04X span=%u\n", st, peek(*m, kRamSt), ramEnd,
                unsigned(ramEnd - st));
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    std::string model, card, kind, streamPath, loaderFile;
    bool doProbe = false, expectRefuse = false;
    for (int i = 1; i < argc; ++i) {
        auto val = [&](std::string* dst) { if (i + 1 < argc) *dst = argv[++i]; };
        if (!std::strcmp(argv[i], "--probe")) doProbe = true;
        else if (!std::strcmp(argv[i], "--expect-refuse")) expectRefuse = true;
        else if (!std::strcmp(argv[i], "--model")) val(&model);
        else if (!std::strcmp(argv[i], "--card")) val(&card);
        else if (!std::strcmp(argv[i], "--kind")) val(&kind);
        else if (!std::strcmp(argv[i], "--stream")) val(&streamPath);
        else if (!std::strcmp(argv[i], "--loader-file")) val(&loaderFile);
        else { std::fprintf(stderr, "unknown argument %s\n", argv[i]); return 2; }
    }
    if (card == "none") card.clear();
    if (model.empty()) { std::fprintf(stderr, "--model required\n"); return 2; }
    if (doProbe) return probe(model, card);

    const bool ml = kind == "ml";
    std::vector<uint8_t> stream;
    if (!matrix::readFile(streamPath, &stream) || stream.size() <= kHeaderLen) {
        std::fprintf(stderr, "cannot read stream %s\n", streamPath.c_str());
        return 2;
    }
    const std::vector<uint8_t> payload(stream.begin() + kHeaderLen, stream.end());

    Run a = runRom(model, card, ml, stream);
    Run b = runLoader(model, card, loaderFile);
    std::printf("ROM:    %s %s\n", a.ok ? "ok" : "REFUSED/FAILED", a.note.c_str());
    std::printf("LOADER: %s %s\n", b.ok ? "ok" : "REFUSED/FAILED", b.note.c_str());

    if (expectRefuse) {
        const bool pass = !a.ok && !b.ok;
        std::printf("RESULT %s (expect refuse)\n", pass ? "PASS" : "FAIL");
        return pass ? 0 : 1;
    }
    if (a.mem.empty() || b.mem.empty()) {
        std::printf("RESULT FAIL (no memory snapshot)\n");
        return 1;
    }

    // The loaded range must hold the payload, in both runs.
    uint32_t loadAt;
    if (ml) {
        loadAt = uint32_t(stream[21]) << 8 | stream[22]; // CE-158 header: load address
    } else {
        loadAt = uint32_t(a.mem[kBasPrgSt]) << 8 | a.mem[kBasPrgSt + 1];
    }
    auto holds = [&](const Run& r) {
        if (loadAt + payload.size() > r.mem.size()) return false;
        return std::equal(payload.begin(), payload.end(), r.mem.begin() + loadAt);
    };
    const bool aHolds = holds(a), bHolds = holds(b);

    const uint32_t userFrom = uint32_t(a.mem[kRamSt]) << 8, userTo = uint32_t(a.mem[kRamEnd]) << 8;
    size_t nUser = 0, nPtr = 0, nSys = 0;
    const std::string dUser = ranges(a.mem, b.mem, userFrom, userTo, &nUser);
    const std::string dPtr = ranges(a.mem, b.mem, 0x7860, 0x7870, &nPtr);
    const std::string dSys = ranges(a.mem, b.mem, 0x7600, 0x8000, &nSys);

    std::printf("load at &%04X, %zu bytes; payload in RAM: rom=%s loader=%s\n", loadAt, payload.size(),
                aHolds ? "yes" : "NO", bHolds ? "yes" : "NO");
    std::printf("BASPRG_ST rom=%04X loader=%04X  BASPRG_END rom=%04X loader=%04X\n",
                a.mem[kBasPrgSt] << 8 | a.mem[kBasPrgSt + 1], b.mem[kBasPrgSt] << 8 | b.mem[kBasPrgSt + 1],
                a.mem[kBasPrgEnd] << 8 | a.mem[kBasPrgEnd + 1], b.mem[kBasPrgEnd] << 8 | b.mem[kBasPrgEnd + 1]);
    std::printf("user RAM &%04X-&%04X diff: %zu bytes %s\n", userFrom, userTo - 1, nUser, dUser.c_str());
    std::printf("pointers &7860-&786F diff: %zu bytes %s\n", nPtr, dPtr.c_str());
    std::printf("system RAM &7600-&7FFF diff (info): %zu bytes %s\n", nSys, dSys.c_str());

    const bool pass = a.ok && b.ok && aHolds && bHolds && nUser == 0 && nPtr == 0;
    std::printf("RESULT %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}

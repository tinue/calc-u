// Headless end-to-end test for the preset loader's `- trace:` step
// (PC1500PresetLoader.cpp) as today's tools run it: the core writes the file
// itself (PC1500TraceFile's file mode). The app's trace -- the same records
// through the library's trace session (LIB-10 §10.7, L-A3) -- is
// debugger_tests.cpp's. Drives applyPC1500Preset() against a real ROM
// (roms/PC-1500_A04.ROM, relative to the repo root) with a preset that
// starts a CPU instruction trace, runs a bit, and stops it, then parses
// the produced file back with a minimal inline reader and checks it is a
// well-formed CALCU1500_TRACE stream (magic/version, SESSION_START first,
// >=1 TRACE_EVENT, SESSION_END last). Skips (not fails) if the ROM is
// missing, same convention as basictyper_tests.cpp / lh5801_tests.cpp's
// test_boot_smoke_real_rom.
//
// Build & run: see tools/run_tests.sh

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/Preset/PresetFile.hpp"
#include "../core/PC1500/PC1500PresetLoader.hpp"
#include "../core/PC1500/PC1500TraceFile.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "../core/PC1600/PC1600PresetLoader.hpp"
#include "PresetTestSupport.hpp"
#include "TempFolder.hpp"
#include "TestRoms.hpp"
#include "TestHarness.hpp"
#include "TraceFileReader.hpp"

namespace {

using tracetest::readTrace;
using tracetest::TraceSummary;
using tracetest::le16;
using tracetest::le32;

const char* kRomPath = "roms/PC-1500_A04.ROM";

bool romPresent() { return testRomSet() != nullptr; }   // the one lookup (LIB-12)

// A preset that explicitly starts and stops a trace produces a complete
// stream: SESSION_START ... TRACE_EVENT(s) ... SESSION_END.
TEST(lib10_ac42_tools_trace_step_produces_wellformed_file) {
    SKIP_UNLESS(romPresent(), (std::string(kRomPath) + " not found relative to cwd (run tests from the repo root)").c_str());
    calcu::testing::TempFolder tmp("calcu1600-trace");
    const std::string dir = tmp.str();

    PresetFile preset;
    std::string err;
    CHECK(parsePresetString(
        "format-version: 1\nmodel: PC-1500A\n"
        "keys:\n"
        "  - trace: t.bin\n"
        "  - wait: 0.3\n"
        "  - trace: off\n",
        dir + "/scratch.pc1500", &preset, &err));

    PC1500Machine machine(preset.variant);
    PresetLoadResult res = applyPC1500Preset(machine, preset, {}, dir, ".", {}, testPresetRoms());
    CHECK(res.ok);

    TraceSummary s = readTrace(dir + "/t.bin");
    CHECK(s.headerOk);
    CHECK(s.firstRecordType == 0x01);   // SESSION_START
    CHECK(s.lastRecordType == 0x03);    // SESSION_END
    CHECK(s.sessionStartCount == 1);
    CHECK(s.sessionEndCount == 1);
    CHECK(s.traceEventCount > 0);
    CHECK(s.allEventPayloads25);
    CHECK(!s.truncated);

}

// A trace left running at the end of the preset is still finalised
// (SESSION_END written, file closed) by runPresetSections()' TraceCloser guard.
TEST(lib10_ac42_tools_trace_left_open_is_auto_closed) {
    SKIP_UNLESS(romPresent(), (std::string(kRomPath) + " not found").c_str());
    calcu::testing::TempFolder tmp("calcu1600-trace");
    const std::string dir = tmp.str();

    PresetFile preset;
    std::string err;
    CHECK(parsePresetString(
        "format-version: 1\nmodel: PC-1500A\n"
        "keys:\n"
        "  - trace: t2.bin\n"
        "  - wait: 0.2\n",
        dir + "/scratch.pc1500", &preset, &err));

    PC1500Machine machine(preset.variant);
    PresetLoadResult res = applyPC1500Preset(machine, preset, {}, dir, ".", {}, testPresetRoms());
    CHECK(res.ok);

    TraceSummary s = readTrace(dir + "/t2.bin");
    CHECK(s.headerOk);
    CHECK(s.firstRecordType == 0x01);
    CHECK(s.lastRecordType == 0x03);   // finalised despite no `- trace: off`
    CHECK(s.traceEventCount > 0);

}

// PC1600Machine's trace draining writes both frame shapes into the SAME
// TRACE.bin (Calc-U-1600's PC-1600 dual-CPU trace support) -- exercised
// here directly via PC1500TraceFile rather than a full preset/GUI round
// trip (no `.pc1600` preset format exists yet). Confirms the
// TRACE_EVENT_Z80 (0x04) record's 29-byte payload survives a write/read
// round trip alongside an ordinary (LH5801-shaped) TRACE_EVENT in the
// same file, both readable by a single linear scan.
TEST(lib10_ac42_z80_frame_shares_one_file_with_lh5801_frame) {
    calcu::testing::TempFolder tmp("calcu1600-trace");
    const std::string dir = tmp.str();
    std::string path = dir + "/z80.bin";

    uint64_t counted = 0;
    {
        std::FILE* fh = std::fopen(path.c_str(), "wb");
        CHECK(fh != nullptr);
        PC1500TraceFile tf(fh); // writes header + SESSION_START

        CpuFrame lh{};
        lh.seqno = 1; lh.pc = 0x1234; lh.cpuId = CPU_ID_LH5803;
        tf.writeFrame(lh);

        Z80CpuFrame z{};
        z.seqno = 2; z.pc = 0x5678; z.cpuId = CPU_ID_SC7852; z.af = 0xAABB;
        tf.writeFrame(z);

        tf.finish(); // writes SESSION_END, closes
        counted = tf.bytesWritten();
    }

    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(buf.size() >= 16);
    CHECK(counted == buf.size()); // what the GUI's size cap reads instead of stat()
    CHECK(le32(buf.data()) == 0x50433135u);
    CHECK(le16(buf.data() + 4) == 3); // version

    size_t i = 16;
    int lh5801Events = 0, z80Events = 0, sessionEnds = 0;
    while (i + 3 <= buf.size()) {
        uint8_t type = buf[i];
        uint16_t len = le16(buf.data() + i + 1);
        CHECK(i + 3 + len <= buf.size());
        if (i + 3 + len > buf.size()) break;
        if (type == 0x02) { CHECK(len == 25); lh5801Events++; }
        if (type == 0x04) { CHECK(len == 29); z80Events++; }
        if (type == 0x03) sessionEnds++;
        i += 3 + len;
    }
    CHECK(lh5801Events == 1);
    CHECK(z80Events == 1);
    CHECK(sessionEnds == 1);

    std::remove(path.c_str());
}

// ── PC-1600 `- trace:` step, end to end ────────────────────────────────
// A PC-1600 preset that starts and stops a trace produces a complete
// stream, and -- because both CPUs run during boot -- it carries BOTH a
// 25-byte LH5803-shaped TRACE_EVENT (0x02) and a 29-byte SC7852-shaped
// TRACE_EVENT_Z80 (0x04) in the one file.
TEST(lib10_ac42_tools_pc1600_trace_step_produces_wellformed_file) {
    PC1600Machine machine;
    SKIP_UNLESS(loadPC1600Roms(machine), "PC-1600 ROM images not found");
    calcu::testing::TempFolder tmp("calcu1600-trace");
    const std::string dir = tmp.str();

    PresetFile preset;
    std::string err;
    // A `type:` step inside the trace window drives the ROM's line editor,
    // which runs on both CPUs -- so the file gets both frame shapes.
    CHECK(parsePresetString(
        "format-version: 1\nmodel: PC-1600\n"
        "keys:\n"
        "  - trace: t.bin\n"
        "  - type: ABC\n"
        "  - wait: 0.3\n"
        "  - trace: off\n",
        dir + "/scratch.pc1600", &preset, &err));

    PresetLoadResult res = applyPC1600Preset(machine, preset, {}, dir);
    CHECK(res.ok);

    TraceSummary s = readTrace(dir + "/t.bin");
    CHECK(s.headerOk);
    CHECK(s.firstRecordType == 0x01);   // SESSION_START
    CHECK(s.lastRecordType == 0x03);    // SESSION_END
    CHECK(s.sessionStartCount == 1);
    CHECK(s.sessionEndCount == 1);
    CHECK(s.traceEventCount > 0);       // LH5803 frames
    CHECK(s.z80EventCount > 0);         // SC7852 frames
    CHECK(s.allEventPayloads25);
    CHECK(s.allZ80Payloads29);
    CHECK(!s.truncated);

}

// A PC-1600 trace left running at the end of the preset is still finalised
// by applyPC1600Preset()'s TraceCloser guard.
TEST(lib10_ac42_tools_pc1600_trace_left_open_is_auto_closed) {
    PC1600Machine machine;
    SKIP_UNLESS(loadPC1600Roms(machine), "PC-1600 ROM images not found");
    calcu::testing::TempFolder tmp("calcu1600-trace");
    const std::string dir = tmp.str();

    PresetFile preset;
    std::string err;
    CHECK(parsePresetString(
        "format-version: 1\nmodel: PC-1600\n"
        "keys:\n"
        "  - trace: t2.bin\n"
        "  - type: ABC\n"
        "  - wait: 0.2\n",
        dir + "/scratch.pc1600", &preset, &err));

    PresetLoadResult res = applyPC1600Preset(machine, preset, {}, dir);
    CHECK(res.ok);

    TraceSummary s = readTrace(dir + "/t2.bin");
    CHECK(s.headerOk);
    CHECK(s.firstRecordType == 0x01);
    CHECK(s.lastRecordType == 0x03);   // finalised despite no `- trace: off`
    CHECK(s.traceEventCount > 0);

}

// Not a trace test, but the same ROM-driven applyPC1500Preset() setup: a
// machine-code `program:` block that isn't all RAM, or runs past &FFFF, fails the
// preset instead of dropping or wrapping bytes and reporting success.
TEST(test_binary_program_outside_ram_fails) {
    SKIP_UNLESS(romPresent(), (std::string(kRomPath) + " not found").c_str());
    calcu::testing::TempFolder tmp("calcu1600-trace");
    const std::string dir = tmp.str();
    {
        std::ofstream bin(dir + "/code.bin", std::ios::binary);
        bin.write("\x01\x02\x03\x04", 4);
    }
    auto run = [&](const char* address) {
        PresetFile preset;
        std::string err;
        CHECK(parsePresetString(std::string("format-version: 1\nmodel: PC-1500A\nprogram:\n  file: code.bin\n"
                                            "  address: ") + address + "\n",
                                dir + "/scratch.pc1500a", &preset, &err));
        PC1500Machine machine(preset.variant);
        return applyPC1500Preset(machine, preset, {}, dir, ".", {}, testPresetRoms());
    };
    PresetLoadResult rom = run("0xC000");
    CHECK(!rom.ok && rom.error.find("not RAM") != std::string::npos);
    PresetLoadResult wrap = run("0xFFFE");
    CHECK(!wrap.ok && wrap.error.find("past &FFFF") != std::string::npos);
    CHECK(run("0x7C01").ok);

}

// A machine-code `file:` with a CE-158 header on a PC-1500: the payload (not the
// 27 header bytes) lands at the header's load address, with no `address:`.
// A headerless file without `address:` is refused at load time.
TEST(test_binary_program_ce158_header) {
    SKIP_UNLESS(romPresent(), (std::string(kRomPath) + " not found").c_str());
    calcu::testing::TempFolder tmp("calcu1600-trace");
    const std::string dir = tmp.str();
    {
        // CE-158 header: 01 'B' "COM", 16-byte name, then big-endian load
        // address, length - 1 and auto-run address (0 = none).
        std::vector<char> header = {0x01, 0x42, 'C', 'O', 'M'};
        header.resize(5 + 16, 0);
        for (uint16_t v : {uint16_t{0x7C10}, uint16_t{4 - 1}, uint16_t{0}}) {
            header.push_back(static_cast<char>(v >> 8));
            header.push_back(static_cast<char>(v & 0xFF));
        }
        std::ofstream bin(dir + "/ce158.bin", std::ios::binary);
        bin.write(header.data(), static_cast<std::streamsize>(header.size()));
        bin.write("\x11\x22\x33\x44", 4);
        std::ofstream raw(dir + "/raw.bin", std::ios::binary);
        raw.write("\x11\x22", 2);
    }
    auto run = [&](const char* file) {
        PresetFile preset;
        std::string err;
        CHECK(parsePresetString(std::string("format-version: 1\nmodel: PC-1500A\nprogram:\n  file: ") + file + "\n",
                                dir + "/scratch.pc1500a", &preset, &err));
        PC1500Machine machine(preset.variant);
        PresetLoadResult res = applyPC1500Preset(machine, preset, {}, dir, ".", {}, testPresetRoms());
        return std::make_pair(res, std::vector<uint8_t>{machine.debugPeek(0x7C10), machine.debugPeek(0x7C13)});
    };
    auto [loaded, bytes] = run("ce158.bin");
    CHECK(loaded.ok);
    CHECK(bytes[0] == 0x11 && bytes[1] == 0x44);
    auto [headerless, unused] = run("raw.bin");
    (void)unused;
    CHECK(!headerless.ok && headerless.error.find("'address' is required") != std::string::npos);
}

} // namespace

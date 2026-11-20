// A trace of a TI machine in the trace
// format, version 2, without key and card records, whoever starts it; one
// trace session of the library. trace_writer_records writes build/trace-test.bin, which the
// CTest CalcU59TraceReader reads with tools/read_trace.py. The byte-for-byte
// comparison with the recorded reference is in
// golden_runs_tests.cpp (it plays a golden-run script).

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "TestRoms.hpp"
#include "../app/TITraceRecorder.hpp"
#include "../app/TraceWriter.hpp"
#include <fstream>
#include <limits>

using namespace cu59;
using namespace std::chrono_literals;
using calcu::sessions::Value;

namespace {

uint16_t le16(const std::vector<uint8_t>& s, size_t i) { return static_cast<uint16_t>(s[i] | (s[i + 1] << 8)); }
uint32_t le32(const std::vector<uint8_t>& s, size_t i) { return le16(s, i) | (static_cast<uint32_t>(le16(s, i + 2)) << 16); }

struct Records {
    std::vector<uint8_t> types;
    uint32_t events = 0, endCount = 0;
    bool whole = false;
};
Records records(const std::vector<uint8_t>& s) {
    Records r;
    size_t pos = 16;
    while (pos + 3 <= s.size()) {
        const uint8_t type = s[pos];
        const uint16_t len = le16(s, pos + 1);
        r.types.push_back(type);
        if (type == 0x02) ++r.events;
        if (type == 0x03) r.endCount = le32(s, pos + 3);
        pos += 3u + len;
    }
    r.whole = pos == s.size();
    return r;
}

std::vector<uint8_t> readBytes(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

constexpr uint64_t kAll = std::numeric_limits<uint64_t>::max();

} // namespace

// The header with TI59, version 2 and the model; written for read_trace.py.
TEST(header_ti59_version_2_and_model) {
    auto m = testroms::makeMachine(ModelId{kTI58C});
    auto rec = makeTITraceRecorder(*m);
    CHECK(rec != nullptr);
    if (!rec) return;
    std::vector<uint8_t> out;
    rec->begin(out);
    for (int i = 0; i < 5; ++i) {
        testroms::run(*m, 500);
        CHECK(rec->take(out, kAll));
    }
    rec->end(out);
    std::ofstream("build/trace-test.bin", std::ios::binary).write(reinterpret_cast<const char*>(out.data()),
                                                                  static_cast<std::streamsize>(out.size()));
    CHECK_EQ(le32(out, 0), 0x54493539u);   // 'TI59'
    CHECK_EQ(le16(out, 4), 2);             // version
    CHECK_EQ(le16(out, 6), 2);             // model TI-58C
    const Records r = records(out);
    CHECK(r.whole);
    CHECK_EQ(r.types.front(), 0x01);       // SESSION_START
    CHECK_EQ(r.types.back(), 0x03);        // SESSION_END
    CHECK_EQ(r.events, 2500u);
    CHECK_EQ(r.endCount, r.events);
}

// Session start and end, an instruction, and a gap where the ring dropped frames.
TEST(records_session_instruction_gap) {
    auto m = testroms::makeMachine(ModelId{kTI59});
    auto rec = makeTITraceRecorder(*m);
    std::vector<uint8_t> out;
    rec->begin(out);
    testroms::run(*m, 3000);   // more than the 1024 frames of the ring: a gap
    CHECK(rec->take(out, kAll));
    rec->end(out);
    const Records r = records(out);
    CHECK(r.whole);
    std::set<uint8_t> kinds(r.types.begin(), r.types.end());
    CHECK(kinds == (std::set<uint8_t>{0x01, 0x02, 0x03, 0x05}));
    CHECK_EQ(r.events, 1024u);
    // A budget: whole records only, the rest waits.
    auto m2 = testroms::makeMachine(ModelId{kTI59});
    auto rec2 = makeTITraceRecorder(*m2);
    std::vector<uint8_t> o2;
    rec2->begin(o2);
    testroms::run(*m2, 100);
    const size_t start = o2.size();
    CHECK(!rec2->take(o2, 3 * TraceWriter::kEventBytes + 5));
    CHECK_EQ(o2.size() - start, 3 * TraceWriter::kEventBytes);
    CHECK(rec2->take(o2, kAll));
}

// No key and no card record, also for a trace with keys and a card (the
// button's, through the session).
TEST(no_key_or_card_records) {
    testroms::SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    r.send(calcu::debugger::kTraceCommand, Value::object({{"button", true}}));
    CHECK(r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic)->running);
    r.tap(83);
    r.send(calcu::media::kStartNewCommand, Value::object({{"place", "card-reader"}, {"template", "implicit:Fresh card"}, {"name", "x"}}));
    r.send(calcu::media::kFeedCommand, Value::object({{"place", "card-reader"}, {"side", 0}}));
    r.runFor(300ms);
    r.send(calcu::media::kEjectCommand, Value::object({{"place", "card-reader"}}));
    r.send(calcu::debugger::kTraceCommand, Value::object({{"off", true}}));
    const auto bytes = readBytes(r.folder.path() / "CALCU59_TRACE.bin");
    const Records rec = records(bytes);
    CHECK(rec.whole && rec.events > 0);
    for (uint8_t t : rec.types) CHECK(t != 0x04);
}

// The button's file is named after the model.
TEST(button_names_by_model) {
    for (ModelId model : {ModelId{kTI59}, ModelId{kTI58}, ModelId{kTI58C}}) {
        auto m = testroms::makeMachine(model);
        auto rec = makeTITraceRecorder(*m);
        CHECK(rec != nullptr);
        if (rec) CHECK_EQ(rec->buttonFileName(), TraceWriter::baseFileName(model));
    }
    CHECK_EQ(TraceWriter::baseFileName(ModelId{kTI59}), std::string("CALCU59_TRACE.bin"));
    CHECK_EQ(TraceWriter::baseFileName(ModelId{kTI58}), std::string("CALCU58_TRACE.bin"));
    CHECK_EQ(TraceWriter::baseFileName(ModelId{kTI58C}), std::string("CALCU58C_TRACE.bin"));
}

// The size limit ends every trace with the library's notice, also a preset's trace.
TEST(limit_ends_with_a_notice) {
    testroms::SessionRig r(testroms::RigOptions{.traceMaxMb = 1});   // 1 MiB for the library
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    r.send(calcu::debugger::kTraceCommand, Value::object({{"file", "limit.bin"}}));
    CHECK(r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic)->running);
    r.send(calcu::sessions::kSpeedCommand, Value::object({{"full", true}}));
    CHECK(r.until([&] { return !r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic)->running; }, 20s));
    bool notice = false;
    for (const auto& n : r.notices()) notice = notice || n.find("reached the size limit of 1 MB") != std::string::npos;
    CHECK(notice);
    const auto bytes = readBytes(r.folder.path() / "limit.bin");
    CHECK(bytes.size() <= 1024u * 1024u);
    CHECK(records(bytes).whole && records(bytes).types.back() == 0x03);
}

// A trace of a preset and the button's are one session, one trace at a
// time.
TEST(preset_and_button_traces_are_one_session) {
    testroms::SessionRig r;
    CHECK(r.until([&] { return r.machine().isKeyReady(); }));
    r.send(calcu::debugger::kTraceCommand, Value::object({{"button", true}}));
    CHECK(r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic)->fileName == "CALCU59_TRACE.bin");
    const std::string preset =
        "format-version: 2\nmodel: TI-59\nslot-1: { name: ML }\nkeys:\n  - trace: preset.bin\n  - key: 2\n  - wait: 200ms\n";
    r.post(calcu::presets::kLoadPresetCommand, Value::object({{"content", preset}, {"name", "t.ti59"}}));
    CHECK(r.until([&] {
        const auto* t = r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic);
        return t && t->running && t->fileName == "preset.bin";
    }, 5s));
    CHECK(records(readBytes(r.folder.path() / "CALCU59_TRACE.bin")).types.back() == 0x03);   // the button's ended first
    CHECK(r.until([&] { return !r.busy(); }, 10s));
    CHECK(!r.state<calcu::debugger::TraceState>(calcu::debugger::kTraceTopic)->running);   // ends with its preset
    CHECK(records(readBytes(r.folder.path() / "preset.bin")).whole);
}

// The trace session, with the test format of TestDebugParts.hpp: "TTRC",
// 0x01, 3 bytes per instruction, 0x03.
#include "DebugRig.hpp"
#include "TestHarness.hpp"
#include "settings/tests/TestProduct.hpp"
#include <filesystem>
#include <fstream>

using namespace calcu::debugger;
using namespace calcu::debugger::test;

namespace {

std::vector<std::uint8_t> fileBytes(const std::string& path) {
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// A complete file: the header, whole records, the end.
bool complete(const std::vector<std::uint8_t>& b) {
    return b.size() >= 6 && b[0] == 'T' && b[4] == 0x01 && b.back() == 0x03 && (b.size() - 6) % 3 == 0;
}
std::uint64_t records(const std::vector<std::uint8_t>& b) { return b.size() < 6 ? 0 : (b.size() - 6) / 3; }

Value file(const std::string& name) { return Value::object({{"file", name}}); }
Value off() { return Value::object({{"off", true}}); }

} // namespace

TEST(trace_records_every_instruction_until_off) {
    DebugRig r;
    CHECK(r.done(r.send("trace", file("t.bin"))));
    const std::uint64_t before = r.target().retired();
    r.advance(20'000);
    const std::uint64_t ran = r.target().retired() - before;
    CHECK(r.done(r.send("trace", off())));
    r.advance(1'000);   // no more records
    const auto b = fileBytes(r.traces / "t.bin");
    CHECK(complete(b));
    CHECK_EQ(records(b), ran);
    CHECK_EQ(b[5], std::uint8_t(0x02));   // the first record
    CHECK_EQ(b[6], std::uint8_t(0));      // its target
}

TEST(ends_with_its_preset_or_new_machine) {
    DebugRig r;
    const CallId l = r.load(pt::header() + "keys:\n  - trace: p.bin\n  - wait: 10ms\n");
    CHECK(r.done(l));
    CHECK(!r.traceState()->running);   // the end of the preset that started it
    CHECK(complete(fileBytes(r.traces / "p.bin")));
    r.advance(1'000);
    // A trace by the command goes on after a preset ends.
    r.send("trace", file("q.bin"));
    CHECK(r.done(r.load(pt::header() + "keys:\n  - wait: 10ms\n")));   // a preset loaded builds a new machine
    CHECK(!r.traceState()->running);
    CHECK(complete(fileBytes(r.traces / "q.bin")));
}

TEST(replaces_file_one_at_a_time) {
    DebugRig r;
    r.traces.write("t.bin", std::string(5000, 'x'));
    r.send("trace", file("t.bin"));
    r.advance(100);
    r.send("trace", file("u.bin"));   // starting a trace ends the one that runs
    const auto t = fileBytes(r.traces / "t.bin");
    CHECK(complete(t));
    CHECK(t.size() < 5000);   // replaced
    CHECK(r.traceState()->running && r.traceState()->fileName == "u.bin");
    r.send("trace", off());
    CHECK(complete(fileBytes(r.traces / "u.bin")));
}

TEST(slash_refused_at_read) {
    DebugRig r;
    for (const char* name : {"a/b.bin", "a\\\\b.bin"}) {
        const CallId l = r.load(pt::header() + "keys:\n  - trace: \"" + name + "\"\n");
        CHECK(!r.done(l));
        CHECK(r.reason(l).find("line 4") != std::string::npos);
        CHECK(r.reason(l).find("without a folder") != std::string::npos);
    }
    const CallId c = r.send("trace", file("a/b.bin"));
    CHECK(!r.done(c));
}

TEST(cannot_make_file_step_fails) {
    DebugRig r;
    std::filesystem::create_directories(std::filesystem::path(r.traces / "x.bin"));   // a folder of that name
    const CallId l = r.load(pt::header() + "keys:\n  - trace: x.bin\n  - wait: 10ms\n");
    CHECK(!r.done(l));
    CHECK(r.reason(l).find("cannot be made") != std::string::npos);
    CHECK(!r.traceState()->running);
}

TEST(button_name_is_products) {
    DebugRig r;
    r.traces.write("TEST_TRACE.bin", std::string(5000, 'x'));
    CHECK(r.done(r.send("trace", Value::object({{"button", true}}))));
    CHECK(r.traceState()->fileName == "TEST_TRACE.bin");
    r.advance(100);
    r.send("trace", off());
    const auto b = fileBytes(r.traces / "TEST_TRACE.bin");
    CHECK(complete(b) && b.size() < 5000);   // each start replaces the file
}

TEST(limit_ends_complete_file_notice) {
    DebugOptions o;
    o.limitMb = 1;
    DebugRig r(o);
    r.notices();
    r.send("trace", file("big.bin"));
    r.advance(2'000'000);   // about 500000 instructions: more than 1 MiB of records
    CHECK(!r.traceState()->running);
    const auto b = fileBytes(r.traces / "big.bin");
    CHECK(complete(b));
    CHECK(b.size() <= 1024u * 1024u);
    CHECK(b.size() > 1024u * 1024u - 8);
    const auto n = r.notices();
    CHECK(n.size() == 1 && n[0] == "The trace big.bin reached the size limit of 1 MB and ended.");
}

TEST(new_machine_log_line) {
    DebugRig r;
    r.send("trace", file("n.bin"));
    r.advance(100);
    r.logLines();
    r.send("model", Value::object({{"name", "test-dot"}}));
    CHECK(!r.traceState()->running);
    CHECK(pt::hasLine(r.logLines(), "The trace n.bin ended: a new machine was built."));
    CHECK(complete(fileBytes(r.traces / "n.bin")));
}

TEST(trace_topic_whoever_started) {
    DebugRig r;
    CHECK(r.traceState() && !r.traceState()->running);
    r.send("trace", file("c.bin"));
    CHECK(r.traceState()->running && r.traceState()->fileName == "c.bin");
    r.send("trace", Value::object({{"button", true}}));
    CHECK(r.traceState()->fileName == "TEST_TRACE.bin");
    r.send("trace", off());
    CHECK(!r.traceState()->running);
    // A preset's: shown while it runs (Paced, looked at between host calls).
    DebugOptions o;
    o.hosting = calcu::sessions::Hosting::Paced;
    DebugRig p(o);
    p.load(pt::header() + "keys:\n  - trace: s.bin\n  - wait: 1s\n");
    bool seen = false;
    for (int i = 0; i < 1000 && p.session->busy(); ++i) {
        p.call();
        if (p.traceState()->running && p.traceState()->fileName == "s.bin") seen = true;
    }
    CHECK(seen);
    CHECK(!p.traceState()->running);
}

TEST(no_writable_folder_step_fails_with_reason) {
    {
        DebugOptions o;
        o.folder = DebugOptions::Folder::None;
        DebugRig r(o);
        const CallId l = r.load(pt::header() + "keys:\n  - trace: t.bin\n");
        CHECK(!r.done(l));
        CHECK(r.reason(l).find("There is no folder for traces.") != std::string::npos);
    }
    {
        DebugOptions o;
        o.folder = DebugOptions::Folder::NotWritable;
        DebugRig r(o);
        const CallId l = r.load(pt::header() + "keys:\n  - trace: t.bin\n");
        CHECK(!r.done(l));
        CHECK(r.reason(l).find("The trace folder cannot be written: it is read-only.") != std::string::npos);   // with the period
        CHECK(r.reason(l).find("read-only..") == std::string::npos);
    }
}

// The trace folder of the places as the settings say at each trace start,
// made and written once; a folder changed after the start is the next trace's.
TEST(trace_folder_read_at_each_trace_start) {
    namespace st = calcu::settings;
    st::test::TempDir dir;
    st::SettingsFile file = st::SettingsFile::defaultsOnly(st::test::testProduct());
    st::HostDirs dirs;
    dirs.frontend = st::FrontendKind::Qt6;
    dirs.home = dir.str();
    const st::Paths paths(file, dirs);
    const auto folder = traceFolderOf(paths);
    const std::string a = (dir.path() / "a").string(), b = (dir.path() / "b").string();
    file.set("trace", "folder", a);
    const auto first = folder();
    CHECK(first && first->ok && first->path == a && std::filesystem::is_directory(a));   // made at the start
    file.set("trace", "folder", b);
    const auto second = folder();
    CHECK(second && second->ok && second->path == b && std::filesystem::is_directory(b));
    // A folder that cannot be made: the reason, a phrase for the refusal's sentence.
    file.set("trace", "folder", "/dev/null/no");
    const auto bad = folder();
    CHECK(bad && !bad->ok && !bad->problem.empty() && bad->problem.back() != '.');
    // Not available now (the Swift frontend's iCloud, the places' callback):
    // asked at each call too.
    bool available = false;
    st::HostDirs swift;
    swift.frontend = st::FrontendKind::Swift;
    swift.home = dir.str();
    swift.saveFolder = a;
    swift.iCloudAvailable = [&available] { return available; };
    file.set("trace", "location", "icloud");
    const st::Paths swiftPaths(file, swift);
    const auto gated = traceFolderOf(swiftPaths);
    CHECK(gated() && !gated()->ok && gated()->problem == "the trace location is not available");
    available = true;
    CHECK(gated() && gated()->ok);
}

// [trace] max-file-size-mb zero or negative (values of the key, no
// minimum) is 50 MiB, as missing is.
TEST(limit_zero_or_negative_is_50_mib) {
    for (long mb : {0L, -3L}) {
        TraceSetup s;
        s.limitMb = [mb] { return mb; };
        CHECK_EQ(TraceSession(s).limitBytes(), std::uint64_t(50) * 1024 * 1024);
    }
    TraceSetup seven;
    seven.limitMb = [] { return 7L; };
    CHECK_EQ(TraceSession(seven).limitBytes(), std::uint64_t(7) * 1024 * 1024);
    CHECK_EQ(TraceSession(TraceSetup{}).limitBytes(), std::uint64_t(50) * 1024 * 1024);
}

TEST(refusal_is_one_sentence_with_its_reason) {
    DebugOptions o;
    o.folder = DebugOptions::Folder::NotWritable;
    DebugRig r(o);
    const CallId c = r.send("trace", file("t.bin"));
    CHECK(!r.done(c));
    CHECK_EQ(r.reason(c), std::string("The trace folder cannot be written: it is read-only."));
}

TEST(ended_trace_in_memory_handed_with_keep_file) {
    DebugOptions o;
    o.folder = DebugOptions::Folder::None;
    o.handOver = true;
    o.services.offered = {calcu::sessions::HostService::KeepFile};
    DebugRig r(o);
    CHECK(r.done(r.send("trace", file("w.bin"))));
    r.advance(1'000);
    const std::size_t mark = r.fe.log.size();
    r.send("trace", off());
    const calcu::sessions::ServiceAsk* ask = nullptr;
    const auto sent = r.fe.since(mark);
    for (const auto& m : sent)
        if (const auto* a = std::get_if<calcu::sessions::ServiceAsk>(&m)) ask = a;
    CHECK(ask != nullptr);
    if (!ask) return;
    CHECK(ask->service == calcu::sessions::HostService::KeepFile);
    CHECK(*ask->args.find("name")->string() == "w.bin");
    CHECK(*ask->args.find("mediaType")->string() == "application/octet-stream");
    CHECK(ask->args.find("kind") && *ask->args.find("kind")->string() == "trace");   // a page lists it as a trace
    const auto& bytes = *ask->args.find("bytes")->bytes();
    CHECK(complete(bytes) && records(bytes) > 0);
    // Without hand-over (Calc-U-1600's Web) the start fails.
    DebugOptions q = o;
    q.handOver = false;
    DebugRig s(q);
    CHECK_EQ(s.reason(s.send("trace", file("w.bin"))), std::string("There is no folder for traces."));
}

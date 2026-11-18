// The loaders' session part (app/SharpLoaderPart): loading with a start, at
// once or as an operation and refused while one runs, one sentence per case
// for the menu, the harness and a preset, and the app session's cases (the
// WAV's choice, the address checked as typed).

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "../../app/HarnessProduct.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "SessionFixture.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"

namespace {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;
namespace c = cu1600::commands;
namespace q = cu1600::requests;
using sessionfixture::SessionFixture;
using sessionfixture::romsPresent;
using sessions::Value;

std::vector<std::uint8_t> readBytes(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// The same tape twice in one WAV: its data chunk's samples doubled.
std::vector<std::uint8_t> doubledTape(const std::vector<std::uint8_t>& wav) {
    for (std::size_t i = 12; i + 8 <= wav.size();) {
        const std::uint32_t size = wav[i + 4] | wav[i + 5] << 8 | wav[i + 6] << 16 | std::uint32_t(wav[i + 7]) << 24;
        if (std::equal(wav.begin() + i, wav.begin() + i + 4, "data")) {
            std::vector<std::uint8_t> out(wav.begin(), wav.begin() + i + 8 + size);
            out.insert(out.end(), wav.begin() + i + 8, wav.begin() + i + 8 + size);
            const std::uint32_t doubled = size * 2, riff = static_cast<std::uint32_t>(out.size() - 8);
            for (int b = 0; b < 4; ++b) {
                out[i + 4 + b] = static_cast<std::uint8_t>(doubled >> (8 * b));
                out[4 + b] = static_cast<std::uint8_t>(riff >> (8 * b));
            }
            return out;
        }
        i += 8 + size + (size & 1);
    }
    return {};
}

// A CE-158 machine-code file: `payload` at `load`, auto-run `start` (0: none).
std::vector<std::uint8_t> ce158Code(const std::vector<std::uint8_t>& payload, std::uint16_t load, std::uint16_t start) {
    std::vector<std::uint8_t> f(27, 0x00);
    f[0] = 0x01;
    f[1] = 0x42;
    f[2] = 'C'; f[3] = 'O'; f[4] = 'M';
    const std::uint16_t wire = static_cast<std::uint16_t>(payload.size() - 1);
    for (auto [at, v] : {std::pair<int, std::uint16_t>{0x15, load}, {0x17, wire}, {0x19, start}}) {
        f[at] = static_cast<std::uint8_t>(v >> 8);
        f[at + 1] = static_cast<std::uint8_t>(v & 0xFF);
    }
    f.insert(f.end(), payload.begin(), payload.end());
    return f;
}

Value file(const std::vector<std::uint8_t>& bytes, const char* name = "code.bin") {
    return Value::object({{"bytes", Value::Bytes(bytes)}, {"name", name}});
}
Value with(Value v, const char* key, Value x) {
    Value::Object o = *v.members();
    o.push_back({key, std::move(x)});
    return Value(std::move(o));
}

const std::vector<std::uint8_t> kRtn4 = {0x9A, 0x9A, 0x9A, 0x9A};   // LH5801 RTN x4

std::vector<std::string> logLines(SessionFixture& f) {
    std::vector<std::string> out;
    for (const auto& p : f.listener.mirror().takeAppended(sessions::kLogTopic))
        if (const auto* l = sessions::payloadAs<sessions::LogLine>(p)) out.push_back(l->text);
    return out;
}

// ---- Loading with a start ----

TEST(run_types_start_and_waits_idle) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    f.runFor(1);
    // A PC-1600 header with an address to start at: Z-80 RET at &C100.
    std::vector<std::uint8_t> code = {0xFF, 0x10, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0xC1, 0x00,
                                      0x00, 0xC1, 0x00, 0x00, 0x0F, 0xC9};   // RET at &C100, start &C100
    const calcu::machine::Ticks before = f.part().machine()->tickCount();
    const sessions::ReplyMessage r = f.run(c::kLoadCode, with(file(code), "run", true));
    CHECK(r.done);
    const auto* loaded = sessions::payloadAs<cu1600::CodeLoadedReply>(r.result);
    CHECK(loaded && loaded->started && loaded->callCommand == "CALL &C100");
    // Typed with ENTER, then idle for 20 frames at least.
    const calcu::machine::Ticks after = f.part().machine()->tickCount();
    CHECK(after - before >= f.part().machine()->descriptor().tickRate / 3);
    CHECK(f.screen().find("CALL &C100") != std::string::npos);
}

TEST(without_start_address_nothing_started) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    std::vector<std::uint8_t> code = {0xFF, 0x10, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0xC1, 0x00,
                                      0xFF, 0xFF, 0x00, 0x00, 0x0F, 0xC9};   // no address to start at
    const sessions::CallId call = f.send(c::kLoadCode, with(file(code), "run", true));
    const auto r = f.takeReply(call);
    CHECK(r && r->done && !f.busy());   // at once: no operation
    const auto* loaded = r ? sessions::payloadAs<cu1600::CodeLoadedReply>(r->result) : nullptr;
    CHECK(loaded && !loaded->started);
}

TEST(bound_reached_done_with_log_line) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    f.runFor(1);
    f.part().loaderPart().setIdleBound(2);
    // JR -2 at &C100: never returns.
    std::vector<std::uint8_t> code = {0xFF, 0x10, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00, 0xC1, 0x00,
                                      0x00, 0xC1, 0x00, 0x00, 0x0F, 0x18, 0xFE};
    (void)logLines(f);
    const sessions::ReplyMessage r = f.run(c::kLoadCode, with(file(code), "run", true));
    CHECK(r.done);
    bool said = false;
    for (const std::string& l : logLines(f))
        if (l.find("not idle after") != std::string::npos) said = true;
    CHECK(said);
}

// ---- At once or as an operation; refused while one runs ----

TEST(load_at_once_not_operations) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    const std::string bas = "10 PRINT 1\n";
    sessions::CallId call = f.send(c::kLoadBasic, file(std::vector<std::uint8_t>(bas.begin(), bas.end()), "a.bas"));
    auto r = f.takeReply(call);
    CHECK(r && r->done && !f.busy());
    call = f.send(c::kLoadCode, with(file(kRtn4), "address", "&C100"));
    r = f.takeReply(call);
    CHECK(r && r->done && !f.busy());
}

TEST(type_and_run_are_operations) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    const std::string bas = "10 PRINT 1\n";
    const sessions::CallId call = f.send(c::kTypeBasic, file(std::vector<std::uint8_t>(bas.begin(), bas.end()), "a.bas"));
    CHECK(!f.takeReply(call) && f.busy());
    for (int i = 0; i < 6000 && f.busy(); ++i) f.call();
    CHECK(!f.busy());
}

TEST(refused_while_another_runs) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    const std::string bas = "10 PRINT 1\n20 PRINT 2\n";
    const Value listing = file(std::vector<std::uint8_t>(bas.begin(), bas.end()), "a.bas");
    f.send(c::kTypeBasic, listing);
    CHECK(f.busy());
    auto r = f.takeReply(f.send(c::kLoadBasic, listing));
    CHECK(r && !r->done && r->reason == sessions::kAnotherOperationRuns);
    r = f.takeReply(f.send(c::kTypeBasic, listing));
    CHECK(r && !r->done);
    r = f.takeReply(f.send(c::kLoadCode, with(file(kRtn4), "address", "&C100")));
    CHECK(r && !r->done && r->reason == sessions::kAnotherOperationRuns);
    const sessions::ReplyMessage k = f.listener.ask(q::kFileKind, Value::object({{"bytes", Value::Bytes(kRtn4)}}));
    CHECK(!k.done && k.reason == sessions::kAnotherOperationRuns);
    for (int i = 0; i < 6000 && f.busy(); ++i) f.call();
}

// The menu (the commands the frontends send) and the harness (the same
// commands through its host; it runs an operation to its end inside one
// command, so it meets a running operation only when one was started
// outside it, as above) are refused while an operation runs.
TEST(menu_and_harness_refused_while_busy) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1600"));
    f.send(calcu::startup::kResetCommand);   // an operation of the menu
    CHECK(f.busy());
    const sessions::ReplyMessage p = f.listener.ask(q::kCodePlan, Value::object({{"bytes", Value::Bytes(kRtn4)}}));
    CHECK(!p.done && p.reason == sessions::kAnotherOperationRuns);
    auto r = f.takeReply(f.send(c::kLoadCode, with(file(kRtn4), "address", "&C100")));
    CHECK(r && !r->done && r->reason == sessions::kAnotherOperationRuns);
    for (int i = 0; i < 6000 && f.busy(); ++i) f.call();
}

// ---- One sentence per case for the menu, the harness and a preset ----

// The same case through the menu's command, the harness and a format-1
// preset: the same sentence; the preset puts its section before it.
TEST(same_sentence_menu_harness_preset) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    const fs::path bin = f.root / "raw.bin";
    {
        std::ofstream out(bin, std::ios::binary);
        out.write(reinterpret_cast<const char*>(kRtn4.data()), static_cast<std::streamsize>(kRtn4.size()));
    }
    const std::string path = bin.generic_string();
    const auto menu = f.takeReply(f.send(c::kLoadCode, file(kRtn4, path.c_str())));
    CHECK(menu && !menu->done);
    const std::string sentence = menu ? menu->reason : std::string();
    CHECK(sentence.rfind(path + ": The file has no machine-code header", 0) == 0);

    auto product = cu1600::makeHarnessProduct();
    std::string error;
    auto server = calcu::harness::Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (server) {
        CHECK_EQ(server->line("c1600 load-code " + path).text, "refused: " + sentence + "\n");
        server->finish();
    }

    const fs::path preset = f.root / "code.pc1500a";
    {
        std::ofstream out(preset);
        out << "format-version: 2\nmodel: PC-1500A\nprogram:\n  file: " << path << "\n";
    }
    // The preset fails at its section's line with the loader's reason.
    const sessions::ReplyMessage r = f.run(calcu::presets::kLoadPresetCommand, Value::object({{"ref", preset.generic_string()}}));
    CHECK(!r.done);
    const std::string want = preset.generic_string() + ", line 3: " + sentence;
    CHECK(r.reason == want);
    if (r.reason != want) std::fprintf(stderr, "  preset: %s\n  menu:   %s\n", r.reason.c_str(), sentence.c_str());
}

// ---- The WAV's choice, the address checked as typed ----

TEST(one_wav_one_image) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    const std::vector<std::uint8_t> wav = readBytes("tests/fixtures/tape/pc1500_tape.wav");
    const sessions::ReplyMessage r = f.listener.ask(q::kFileKind, Value::object({{"bytes", Value::Bytes(wav)}}));
    const auto* k = sessions::payloadAs<cu1600::FileKindReply>(r.result);
    CHECK(k && k->kind == "basic-pc1500" && k->tapeFiles.size() == 1);
    const sessions::ReplyMessage plain = f.listener.ask(q::kFileKind, Value::object({{"bytes", Value::Bytes(kRtn4)}}));
    const auto* p = sessions::payloadAs<cu1600::FileKindReply>(plain.result);
    CHECK(p && p->kind == "headerless" && p->tapeFiles.empty());
}

TEST(two_files_listed) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    const std::vector<std::uint8_t> two = doubledTape(readBytes("tests/fixtures/tape/pc1500_tape.wav"));
    const sessions::ReplyMessage r = f.listener.ask(q::kFileKind, Value::object({{"bytes", Value::Bytes(two)}}));
    const auto* k = sessions::payloadAs<cu1600::FileKindReply>(r.result);
    CHECK(k && k->tapeFiles.size() == 2 && k->tapeFiles[0].rfind("TAPEFIX", 0) == 0);
}

TEST(address_checked_as_typed) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    SessionFixture f;
    CHECK(f.start("PC-1500A"));
    auto plan = [&](const char* address) {
        const sessions::ReplyMessage r = f.listener.ask(q::kCodePlan, with(file(kRtn4), "address", address));
        const auto* p = sessions::payloadAs<cu1600::CodePlanReply>(r.result);
        return p ? *p : cu1600::CodePlanReply{};
    };
    cu1600::CodePlanReply p = plan("");
    CHECK(p.needsAddress && p.length == 4 && p.cpuNote.empty() && p.placeholder == "hex, e.g. &7C01");
    CHECK(plan("zz").refusal == "Not a hex address (&0000-&FFFF).");
    CHECK(!plan("&FFFE").refusal.empty());
    p = plan("&7C01");
    CHECK(p.refusal.empty() && !p.where.empty());
    const sessions::ReplyMessage r = f.run(c::kLoadCode, with(file(kRtn4), "address", "&7C01"));
    const auto* loaded = sessions::payloadAs<cu1600::CodeLoadedReply>(r.result);
    CHECK(r.done && loaded && loaded->summary == "Loaded 4 bytes at &7C01–&7C04" && loaded->callCommand == "CALL &7C01");

    CHECK(f.model("PC-1600"));
    p = plan("");
    CHECK(p.needsAddress && p.proposed == "&C0C5" && !p.cpuNote.empty());
    p = plan("&C0C5");
    CHECK(p.refusal.empty());
    const sessions::ReplyMessage r2 = f.run(c::kLoadCode, with(file(kRtn4), "address", "&C0C5"));
    const auto* l2 = sessions::payloadAs<cu1600::CodeLoadedReply>(r2.result);
    CHECK(r2.done && l2 && l2->summary == "Loaded 4 bytes at &C0C5–&C0C8 (S0)" && l2->callCommand == "CALL &C0C5");
    p = plan("&F800");   // into the work area: a warning, still loadable
    CHECK(p.refusal.empty() && p.where.find("work area") != std::string::npos);
}

} // namespace

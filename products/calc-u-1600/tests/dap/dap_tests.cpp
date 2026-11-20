// The DAP server of app/dap/ -- its JSON and
// framing, the part (listening, status, held messages, ending) and the
// session (set-up, Build & Load, requests, events) -- driven through a test
// transport (bytes in and out, on the test's thread) in a session of
// app/tests/SessionFixture.hpp, one host call at a time. The ten scenarios of
// tools/dap_smoke.py run against the harness as CalcU1600DapSmoke.
// A program of its own (CalcU1600DapTests), so that CalcU1600CoreTests' Frequent budget
// stays as it is.

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "SessionFixture.hpp"
#include "TestHarness.hpp"
#include "app/dap/DapConfig.hpp"
#include "app/dap/DapJson.hpp"
#include "app/dap/DapPart.hpp"
#include "app/dap/DapSession.hpp"
#include "app/dap/DapWire.hpp"
#include "debugger/DebuggerPart.hpp"
#include "debugger/RunControl.hpp"
#include "startup/StartupPart.hpp"

namespace {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;
using sessionfixture::SessionFixture;
using cu1600::dap::Value;

const char* kMemtestBin = "tests/fixtures/listings/sdas-lh5801/memtest_stock.bin";
const char* kMemtestRst = "tests/fixtures/listings/sdas-lh5801/memtest.rst";
const char* kMemtestAsm = "tests/fixtures/listings/sdas-lh5801/memtest.asm";
const char* kPc1500Preset = "examples/startup/default-pc1500.pc1500";

struct TestTransport final : cu1600::dap::DapTransport {
    std::optional<std::uint16_t> listening;
    std::optional<std::string> refuse;   // listen() fails with it
    std::string busy, out;
    int dropped = 0, quits = 0;
    std::optional<std::string> listen(std::uint16_t port, std::string b) override {
        if (refuse) return refuse;
        listening = port;
        busy = std::move(b);
        return std::nullopt;
    }
    void close() override { listening.reset(); }
    void send(std::string bytes) override { out += bytes; }
    void dropClient() override { ++dropped; }
    void quitProgram() override { ++quits; }
};

Value obj(std::initializer_list<Value::Member> m) { return Value::object(m); }

// A DAP client over the test transport, as VS Code is one over a socket.
struct Client {
    SessionFixture& fx;
    cu1600::dap::DapPart& part;
    TestTransport t;
    cu1600::dap::DapReader reader;
    std::int64_t seq = 1;
    std::vector<Value> events;
    std::map<std::int64_t, Value> responses;
    std::vector<std::string> output;   // the `output` events' text

    explicit Client(SessionFixture& f) : fx(f), part(*f.part().dapPart()) {
        part.setTransport(&t);
        part.setRunPort(32168);
    }
    ~Client() { part.setTransport(nullptr); }
    void connect() { part.take(cu1600::dap::DapInput{cu1600::dap::DapInput::Kind::Connected, {}}); }
    void gone() { part.take(cu1600::dap::DapInput{cu1600::dap::DapInput::Kind::Disconnected, {}}); }
    std::int64_t send(const std::string& command, Value args = Value(Value::Object{})) {
        const std::int64_t s = seq++;
        const Value msg = obj({{"seq", Value(s)}, {"type", Value("request")}, {"command", Value(command)}, {"arguments", args}});
        part.take(cu1600::dap::DapInput{cu1600::dap::DapInput::Kind::Bytes, cu1600::dap::frame(cu1600::dap::writeJson(msg))});
        return s;
    }
    void pump() {
        reader.feed(t.out);
        t.out.clear();
        while (auto body = reader.next()) {
            auto v = cu1600::dap::parseJson(*body);
            if (!v) continue;
            if (cu1600::dap::textOf(*v, "type") == "response") {
                responses[cu1600::dap::intOf(*v, "request_seq")] = *v;
            } else {
                if (cu1600::dap::textOf(*v, "event") == "output")
                    if (const Value* b = v->find("body")) output.push_back(cu1600::dap::textOf(*b, "output"));
                events.push_back(*v);
            }
        }
    }
    // The response to `s`, waited for over at most `calls` host calls.
    std::optional<Value> response(std::int64_t s, int calls = 3000) {
        for (int i = 0;; ++i) {
            pump();
            auto it = responses.find(s);
            if (it != responses.end()) {
                Value r = it->second;
                responses.erase(it);
                return r;
            }
            if (i >= calls) return std::nullopt;
            fx.call();
        }
    }
    // The body of a request that succeeded (an empty object otherwise, with a line).
    Value request(const std::string& command, Value args = Value(Value::Object{}), int calls = 3000) {
        const auto r = response(send(command, std::move(args)), calls);
        if (!r || !cu1600::dap::boolOf(*r, "success")) {
            std::fprintf(stderr, "  %s: %s\n", command.c_str(), r ? cu1600::dap::textOf(*r, "message").c_str() : "no response");
            return Value(Value::Object{});
        }
        const Value* b = r->find("body");
        return b ? *b : Value(Value::Object{});
    }
    std::string refusal(const std::string& command, Value args = Value(Value::Object{})) {
        const auto r = response(send(command, std::move(args)));
        if (!r) return "(no response)";
        return cu1600::dap::boolOf(*r, "success") ? std::string() : cu1600::dap::textOf(*r, "message");
    }
    std::optional<Value> event(const std::string& name, int calls = 1200) {
        for (int i = 0;; ++i) {
            pump();
            for (auto it = events.begin(); it != events.end(); ++it)
                if (cu1600::dap::textOf(*it, "event") == name) {
                    const Value* b = it->find("body");
                    Value body = b ? *b : Value(Value::Object{});
                    events.erase(it);
                    return body;
                }
            if (i >= calls) return std::nullopt;
            fx.call();
        }
    }
    int count(const std::string& name) {
        pump();
        int n = 0;
        for (const Value& e : events) n += cu1600::dap::textOf(e, "event") == name;
        return n;
    }
    bool said(const std::string& part_) {
        pump();
        for (const auto& o : output)
            if (o.find(part_) != std::string::npos) return true;
        return false;
    }
    void begin() {
        connect();
        request("initialize", obj({{"adapterID", Value("c1600")}}));
    }
    calcu::debugger::RunControl& rc() { return fx.part().debuggerPart().runControl(); }
};

Value memtestProgram(const char* after) {
    return obj({{"bin", Value(fs::absolute(kMemtestBin).string())},
                {"listing", Value(fs::absolute(kMemtestRst).string())},
                {"address", Value("0x40C5")},
                {"after", Value(after)}});
}

int lineOfTop(Client& c, int thread = 1) {
    const Value st = c.request("stackTrace", obj({{"threadId", Value(thread)}, {"startFrame", Value(0)}, {"levels", Value(1)}}));
    const Value* frames = st.find("stackFrames");
    if (!frames || !frames->elements() || frames->elements()->empty()) return -1;
    return static_cast<int>(cu1600::dap::intOf(frames->elements()->front(), "line"));
}

bool started(SessionFixture& fx, const char* model = "PC-1500A") {
    fx.start(model);
    return fx.part().machine() != nullptr;
}

// ── JSON and framing ─────────────────────────────────────────────────────

TEST(dap_json_round_trip) {
    const std::string text = R"({"seq":3,"type":"request","arguments":{"n":-12,"x":1.5,"b":true,"z":null,"a":[1,"t\n\"é"]}})";
    const auto v = cu1600::dap::parseJson(text);
    CHECK(v.has_value());
    CHECK_EQ(cu1600::dap::intOf(*v, "seq"), 3);
    const Value* args = v->find("arguments");
    CHECK(args && cu1600::dap::intOf(*args, "n") == -12 && cu1600::dap::boolOf(*args, "b"));
    CHECK(args && args->find("x") && args->find("x")->real() && *args->find("x")->real() == 1.5);
    CHECK_EQ(cu1600::dap::writeJson(*v), text);
    CHECK(!cu1600::dap::parseJson("{\"a\":}"));
    CHECK(!cu1600::dap::parseJson("[1,2"));
    CHECK_EQ(cu1600::dap::unbase64(cu1600::dap::base64("\x01\xFFmemory")), std::string("\x01\xFFmemory"));
}

TEST(dap_wire_frames_across_reads) {
    cu1600::dap::DapReader r;
    const std::string two = cu1600::dap::frame("{\"a\":1}") + "X-Garbage: 1\r\n\r\n" + cu1600::dap::frame("{\"b\":2}");
    for (char c : two) r.feed(std::string(1, c));   // a byte at a time
    CHECK_EQ(r.next().value_or(""), std::string("{\"a\":1}"));
    CHECK_EQ(r.next().value_or(""), std::string("{\"b\":2}"));
    CHECK(!r.next());
}

// ── Listening, second client, status ─────────────────────────────────────

TEST(listens_on_127_0_0_1_by_setting_or_run_port) {
    SessionFixture fx;
    fx.open();
    cu1600::dap::DapPart* part = fx.part().dapPart();
    CHECK(part != nullptr);   // in the app layer of a native host
    if (!part) return;
    TestTransport t;
    part->setTransport(&t);
    CHECK(!t.listening);   // dap-enabled is off by default
    CHECK_EQ(part->status().status, std::string("Off"));
    fx.settings.file().set("debug", "dap-enabled", "true");
    fx.settings.file().set("debug", "dap-port", "40001");
    fx.call();   // the settings changed: the part follows at the next call
    CHECK(t.listening && *t.listening == 40001);
    CHECK_EQ(part->status().status, std::string("Listening on 127.0.0.1:40001"));
    CHECK(!part->setRunPort(40002));   // --dap: for this run, no setting changed
    CHECK(t.listening && *t.listening == 40002);
    CHECK_EQ(fx.settings.file().integer("debug", "dap-port"), 40001);
    part->setTransport(nullptr);
}

TEST(port_taken_status_says_so) {
    SessionFixture fx;
    fx.open();
    TestTransport t;
    t.refuse = "Address already in use";
    fx.part().dapPart()->setTransport(&t);
    fx.part().dapPart()->setRunPort(40003);
    CHECK_EQ(fx.part().dapPart()->status().status, std::string("Port 40003 not available: Address already in use"));
    fx.part().dapPart()->setTransport(nullptr);
}

TEST(second_client_told_and_closed) {
    SessionFixture fx;
    fx.open();
    Client c(fx);
    // The host closes a second connection after these bytes (Qt6: DapSocket,
    // the harness: LocalStream).
    cu1600::dap::DapReader r;
    r.feed(c.t.busy);
    const auto body = r.next();
    CHECK(body && body->find("Calc-U-1600 already has a debugger attached") != std::string::npos);
    CHECK(body && body->find("\"event\":\"output\"") != std::string::npos);
}

TEST(status_texts_on_the_topic) {
    SessionFixture fx;
    fx.open();
    Client c(fx);
    fx.call();
    auto status = [&] {
        const auto* s = fx.state<cu1600::dap::DapStatus>(cu1600::dap::kDapTopic);
        return s ? s->status : std::string("(none)");
    };
    CHECK_EQ(status(), std::string("Listening on 127.0.0.1:32168"));
    c.connect();
    fx.call();
    CHECK_EQ(status(), std::string("Client connected on 127.0.0.1:32168"));
    const auto* s = fx.state<cu1600::dap::DapStatus>(cu1600::dap::kDapTopic);
    CHECK(s && s->client && s->port == 32168);
    c.gone();
    fx.call();
    CHECK_EQ(status(), std::string("Listening on 127.0.0.1:32168"));
}

TEST(port_out_of_range_not_listening) {
    SessionFixture fx;
    fx.open();
    TestTransport t;
    fx.settings.file().set("debug", "dap-enabled", "true");   // also over the setting: off for the run
    fx.part().dapPart()->setTransport(&t);
    CHECK(t.listening);
    const sessions::ReplyMessage r = fx.run(cu1600::dap::kDapPortCommand, sessions::Value::object({{"port", 80}}));
    CHECK(!r.done && r.reason == cu1600::dap::kPortRange);
    CHECK(!t.listening);
    CHECK_EQ(fx.part().dapPart()->status().status, std::string("Off"));
    fx.call();
    bool noticed = false;
    for (const auto& n : fx.notices()) noticed = noticed || n == cu1600::dap::kPortRange;
    CHECK(noticed);
    fx.part().dapPart()->setTransport(nullptr);
}

// ── The start of a DAP session ───────────────────────────────────────────

TEST(initialize_pauses_configuration_done_resumes) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    CHECK(c.event("initialized").has_value());
    fx.runFor(0.1);
    CHECK(c.rc().paused());
    c.request("attach");
    CHECK(c.rc().paused());   // the set-up leaves it paused
    c.request("configurationDone");
    fx.call();
    CHECK(!c.rc().paused());
    CHECK_EQ(c.count("stopped"), 0);   // neither pause is told
}

TEST(stop_on_entry_stays_paused_entry) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.request("attach", obj({{"stopOnEntry", Value(true)}}));
    c.request("configurationDone");
    const auto stop = c.event("stopped");
    CHECK(stop && cu1600::dap::textOf(*stop, "reason") == "entry" && cu1600::dap::boolOf(*stop, "allThreadsStopped"));
    CHECK(c.rc().paused());
}

TEST(project_block_is_the_base_keys_win) {
    SessionFixture fx;
    fx.open();
    const fs::path project = fx.root / "debug.pc1500";
    {
        std::ofstream f(project);
        f << "format-version: 2\nmodel: PC-1500\ndebug:\n  stopOnEntry: true\n  command: RUN\n"
             "  program:\n    bin: a.bin\n    address: &4000\n    after: call\n"
             "  listings:\n    - a.rst\n";
    }
    const Value args = obj({{"project", Value(project.string())},
                            {"command", Value("CALL &4000")},
                            {"program", obj({{"after", Value("none")}})},
                            {"listings", Value(Value::Array{Value("b.rst")})},
                            {"vscodeKey", Value(1)}});
    Value config;
    std::string error;
    CHECK(cu1600::dap::effectiveConfig(args, fx.part().presetsPart().readSetup(), &fx.part().presetBundle(), &config, &error));
    CHECK_EQ(cu1600::dap::textOf(config, "preset"), project.string());   // `project` is the preset
    CHECK_EQ(cu1600::dap::textOf(config, "command"), std::string("CALL &4000"));   // the client's key wins
    CHECK(cu1600::dap::boolOf(config, "stopOnEntry"));
    const Value* program = config.find("program");
    CHECK(program && cu1600::dap::textOf(*program, "after") == "none");   // merged key by key
    CHECK(program && cu1600::dap::textOf(*program, "address") == "&4000");
    CHECK(program && cu1600::dap::textOf(*program, "bin").find("a.bin") != std::string::npos);
    const Value* listings = config.find("listings");
    CHECK(listings && listings->elements() && listings->elements()->size() == 2);   // added
    // A launch configuration's own `preset` wins over the project.
    const Value own = obj({{"project", Value(project.string())}, {"preset", Value("other.pc1500")}});
    CHECK(cu1600::dap::effectiveConfig(own, fx.part().presetsPart().readSetup(), &fx.part().presetBundle(), &config, &error));
    CHECK_EQ(cu1600::dap::textOf(config, "preset"), std::string("other.pc1500"));
}

TEST(unknown_key_passed_over_numbers_by_preset_rule_bare_hex_refused) {
    std::string error;
    CHECK(cu1600::dap::checkAttach(obj({{"vscodeKey", Value("x")}, {"stopOnEntry", Value(true)}}),
                                   debug::attach::kBlockKeys, &error));
    std::uint32_t n = 0;
    CHECK(cu1600::dap::numberOf(Value("&40C5"), 0xFFFF, &n) && n == 0x40C5);
    CHECK(cu1600::dap::numberOf(Value("0x40C5"), 0xFFFF, &n) && n == 0x40C5);
    CHECK(cu1600::dap::numberOf(Value("16581"), 0xFFFF, &n) && n == 16581);
    CHECK(cu1600::dap::numberOf(Value(16581), 0xFFFF, &n) && n == 16581);
    CHECK(!cu1600::dap::numberOf(Value("40C5"), 0xFFFF, &n));
    CHECK(!cu1600::dap::checkAttach(obj({{"address", Value("40C5")}}), debug::attach::kProgramKeys, &error));
    CHECK(error.rfind("Bad \"address\": ", 0) == 0);
}

TEST(bad_key_refused_with_reason) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.request("attach");
    const std::string bin = fs::absolute(kMemtestBin).string();
    CHECK(c.refusal("attach", obj({{"program", obj({{"listing", Value("x.rst")}})}})).rfind("Bad \"bin\"", 0) == 0);
    CHECK(c.refusal("c1600/load", obj({{"bin", Value(bin)}, {"after", Value("foo")}})).rfind("Bad \"after\"", 0) == 0);
    CHECK(c.refusal("c1600/load", obj({{"bin", Value(bin)}, {"cpu", Value("x86")}})).rfind("Bad \"cpu\"", 0) == 0);
    CHECK(c.refusal("c1600/load", obj({{"bin", Value(bin)}, {"slot", Value(2)}})).rfind("Bad \"slot\": not a key of \"program\":", 0) == 0);
    CHECK(c.refusal("restart", obj({{"arguments", obj({{"reset", Value("sometimes")}})}})).rfind("Bad \"reset\"", 0) == 0);
}

// ── The set-up and Build & Load ──────────────────────────────────────────

TEST(boot_debug_with_program_refused) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    const std::string why = c.refusal("attach", obj({{"boot", Value("debug")}, {"program", memtestProgram("none")}}));
    CHECK(why.find("\"boot\": \"debug\"") != std::string::npos);
    CHECK(c.refusal("attach", obj({{"boot", Value("debug")}, {"command", Value("RUN")}})).find("\"boot\"") != std::string::npos);
}

TEST(preset_loaded_then_paused_command_typed_when_running) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.request("attach", obj({{"preset", Value(fs::absolute(kPc1500Preset).string())}, {"command", Value("PRINT 6*7")}}));
    CHECK(fx.part().machine() && fx.part().machine()->model().value == "PC-1500");   // the preset's machine
    CHECK(c.rc().paused());
    fx.runFor(0.5);
    CHECK(fx.screen().find("42") == std::string::npos);   // not typed while paused
    c.request("configurationDone");
    fx.runUntilTyped(10);
    fx.runFor(1);
    CHECK(fx.screen().find("42") != std::string::npos);
}

TEST(reset_stops_at_the_reset_vector_over_when_it_stops) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.request("attach", obj({{"reset", Value("allReset")}, {"stopOnEntry", Value(true)}}));   // answered: the reset is over
    c.request("configurationDone");
    const auto stop = c.event("stopped");
    CHECK(stop && cu1600::dap::textOf(*stop, "reason") == "entry");
    const Value st = c.request("stackTrace", obj({{"threadId", Value(1)}, {"levels", Value(1)}}));
    const Value* frames = st.find("stackFrames");
    CHECK(frames && frames->elements() &&
          cu1600::dap::textOf(frames->elements()->front(), "instructionPointerReference") == "0xE000");
    // Then stepping and breakpoints work normally.
    c.request("stepIn", obj({{"threadId", Value(1)}, {"granularity", Value("instruction")}}));
    CHECK(c.event("stopped").has_value());
}

TEST(listings_bound_to_their_cpu_with_qualifier_told) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    // memtest's listing on a machine that does not hold memtest: bound, and
    // the client told how many lines differ.
    c.request("attach", obj({{"listings", Value(Value::Array{obj({{"path", Value(fs::absolute(kMemtestRst).string())},
                                                                   {"cpu", Value("lh5801")}})})}}));
    CHECK(c.said("lines don't match memory"));
}

TEST(build_and_load_entry_and_call_in_setup_waits_for_configuration_done) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.request("attach", obj({{"preset", Value(fs::absolute(kPc1500Preset).string())}, {"program", memtestProgram("call")}}));
    CHECK(c.said("Clean start: preset"));
    CHECK(c.said("Loaded memtest_stock.bin at 40C5-417C; start with CALL &40C5"));
    const std::string asmPath = fs::absolute(kMemtestAsm).string();
    const Value bps = c.request("setBreakpoints", obj({{"source", obj({{"path", Value(asmPath)}})},
                                                        {"breakpoints", Value(Value::Array{obj({{"line", Value(74)}})})}}));
    const Value* list = bps.find("breakpoints");
    CHECK(list && list->elements() && cu1600::dap::boolOf(list->elements()->front(), "verified"));
    fx.runFor(1);
    CHECK_EQ(c.count("stopped"), 0);   // nothing runs before configurationDone, so no breakpoint near the entry is missed
    c.request("configurationDone");
    const auto stop = c.event("stopped");
    CHECK(stop && cu1600::dap::textOf(*stop, "reason") == "breakpoint");   // the breakpoint at the entry is not missed
    CHECK_EQ(lineOfTop(c), 74);
}

TEST(entry_rules_and_headerless_without_address) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.request("attach", obj({{"preset", Value(fs::absolute(kPc1500Preset).string())}}));
    const std::string bin = fs::absolute(kMemtestBin).string(), rst = fs::absolute(kMemtestRst).string();
    // No address: the lowest listing line with bytes; the listing's ENTRY.
    Value b = c.request("c1600/load", obj({{"bin", Value(bin)}, {"listing", Value(rst)}, {"cleanStart", Value(false)}}));
    CHECK_EQ(cu1600::dap::textOf(b, "start"), std::string("0x40C5"));
    CHECK_EQ(cu1600::dap::textOf(b, "entry"), std::string("0x40C5"));
    b = c.request("c1600/load",
                  obj({{"bin", Value(bin)}, {"listing", Value(rst)}, {"entry", Value("MEMTEST")}, {"cleanStart", Value(false)}}));
    CHECK_EQ(cu1600::dap::textOf(b, "entry"), std::string("0x40CC"));
    CHECK_EQ(cu1600::dap::textOf(b, "call"), std::string("CALL &40CC"));
    b = c.request("c1600/load", obj({{"bin", Value(bin)}, {"entry", Value(0x40D0)}, {"address", Value("&40C5")},
                                         {"cleanStart", Value(false)}}));
    CHECK_EQ(cu1600::dap::textOf(b, "entry"), std::string("0x40D0"));
    // Headerless, no address, no listing.
    const std::string why = c.refusal("c1600/load", obj({{"bin", Value(bin)}, {"cleanStart", Value(false)}}));
    CHECK(why.find("needs an address") != std::string::npos && why.find("listing") != std::string::npos);
}

TEST(startup_preset_for_another_model_refused) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    fx.settings.file().set("preset", "PC-1500A", fs::absolute("tests/fixtures/presets/startup-pc1600.pc1600").string());
    Client c(fx);
    c.begin();
    c.request("attach");
    const std::string why = c.refusal("c1600/load", memtestProgram("none"));
    CHECK(why.rfind("Clean start failed: ", 0) == 0 && why.find("PC-1600") != std::string::npos);
}

TEST(rom_project_load_redoes_the_setup) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.request("attach", obj({{"preset", Value(fs::absolute(kPc1500Preset).string())}, {"stopOnEntry", Value(true)}}));
    c.request("configurationDone");
    CHECK(c.event("stopped").has_value());
    const auto generation = fx.session().machineSession().generation();
    c.request("c1600/load");   // no program: the set-up again (the preset rebuilds the machine)
    CHECK(fx.session().machineSession().generation() != generation);
    const auto stop = c.event("stopped");   // and then as at configurationDone: stopOnEntry
    CHECK(stop && cu1600::dap::textOf(*stop, "reason") == "entry");
}

// ── Held messages, ending ────────────────────────────────────────────────

TEST(messages_held_during_an_operation_answered_in_order) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    const auto attach = c.send("attach", obj({{"preset", Value(fs::absolute(kPc1500Preset).string())}}));
    CHECK(fx.busy());   // the set-up is an operation of the session
    const auto threads = c.send("threads");
    const auto evaluate = c.send("evaluate", obj({{"expression", Value("1+1")}}));
    c.pump();
    CHECK(c.responses.empty());   // held while it runs
    const auto e = c.response(evaluate);
    const auto a = c.response(attach, 0);
    const auto t = c.response(threads, 0);
    CHECK(e && a && t);
    // Answered in the order of arrival: the server's seq of the responses.
    if (e && a && t) {
        CHECK(cu1600::dap::intOf(*a, "seq") < cu1600::dap::intOf(*t, "seq"));
        CHECK(cu1600::dap::intOf(*t, "seq") < cu1600::dap::intOf(*e, "seq"));
        CHECK(cu1600::dap::boolOf(*t, "success"));
    }
}

TEST(disconnect_closed_socket_and_dialog_end_the_session) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    calcu::debugger::BreakpointTable& table = fx.part().debuggerPart().runControl().table();
    auto dapBreakpoints = [&] {
        int n = 0;
        for (const auto& b : table.breakpoints()) n += b.setter == cu1600::dap::kDapSetter;
        return n;
    };
    for (int way = 0; way < 3; ++way) {
        Client c(fx);
        c.begin();
        c.request("attach");
        c.request("setInstructionBreakpoints",
                  obj({{"breakpoints", Value(Value::Array{obj({{"instructionReference", Value("0xE2AA")}})})}}));
        CHECK(dapBreakpoints() == 1);
        CHECK(c.rc().paused());
        if (way == 0) {
            c.request("disconnect");
            CHECK(c.t.dropped == 1);
        } else if (way == 1) {
            c.gone();   // the socket closed
        } else {
            const sessions::ReplyMessage r = fx.run(cu1600::dap::kDapDisconnectCommand);   // Settings ▸ Debugger ▸ Disconnect
            CHECK(r.done);
            CHECK(c.event("terminated", 0).has_value());
            CHECK(c.said("Disconnected from the app"));
        }
        fx.call();
        CHECK(dapBreakpoints() == 0);   // its breakpoints and watches are gone
        CHECK(!c.rc().paused());        // the machine runs on
        CHECK(!fx.part().dapPart()->hasClient());
    }
    CHECK_EQ(fx.run(cu1600::dap::kDapDisconnectCommand).reason, std::string(cu1600::dap::kNoClient));
}

TEST(request_finished_before_the_end) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.send("attach", obj({{"preset", Value(fs::absolute(kPc1500Preset).string())}}));
    CHECK(fx.busy());
    c.gone();   // while the set-up runs
    CHECK(c.rc().paused() || fx.busy());
    for (int i = 0; i < 3000 && fx.busy(); ++i) fx.call();
    CHECK(fx.part().machine() && fx.part().machine()->model().value == "PC-1500");   // finished
    fx.call();
    CHECK(!c.rc().paused());   // then the session ended
}

// ── A new machine ────────────────────────────────────────────────────────

TEST(model_switch_keeps_the_session_invalidated_and_line) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.request("attach");
    c.request("configurationDone");
    CHECK(fx.model("PC-1600"));
    fx.call();
    CHECK(c.event("invalidated", 10).has_value());
    CHECK(c.said("The machine was rebuilt; breakpoints re-applied."));
    const Value t = c.request("threads");
    const Value* list = t.find("threads");
    CHECK(list && list->elements() && list->elements()->size() == 2);   // the session goes on with the PC-1600
}

// ── Requests ─────────────────────────────────────────────────────────────

TEST(requests_memory_references_unsupported) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx, "PC-1600"));
    Client c(fx);
    c.begin();
    c.request("attach", obj({{"stopOnEntry", Value(true)}}));
    c.request("configurationDone");
    CHECK(c.event("stopped").has_value());
    const Value t = c.request("threads");
    const Value* threads = t.find("threads");
    CHECK(threads && threads->elements() && threads->elements()->size() == 2);
    // Thread 2 (the LH5803) above the 16 bits, ME1 at 0x100000.
    CHECK_EQ(cu1600::dap::memoryReference(2, 0x1234), std::string("0x21234"));
    CHECK_EQ(cu1600::dap::memoryReference(1, 0x1234, true), std::string("0x101234"));
    int thread = 0;
    std::uint16_t addr = 0;
    bool me1 = false;
    CHECK(cu1600::dap::parseMemoryReference("0x121234", &thread, &addr, &me1) && thread == 2 && addr == 0x1234 && me1);
    const Value st = c.request("stackTrace", obj({{"threadId", Value(1)}, {"levels", Value(3)}}));
    const Value* frames = st.find("stackFrames");
    CHECK(frames && frames->elements() && !frames->elements()->empty());
    const Value scopes = c.request("scopes", obj({{"frameId", Value(1000)}}));
    const Value vars = c.request("variables", obj({{"variablesReference", Value(1000 * 4 + 1)}}));
    const Value* v = vars.find("variables");
    bool banks = false;
    if (v && v->elements())
        for (const Value& x : *v->elements()) banks = banks || cu1600::dap::textOf(x, "name") == "Banks";
    CHECK(banks);
    const Value bankVars = c.request("variables", obj({{"variablesReference", Value(1000 * 4 + 3)}}));
    const Value* bv = bankVars.find("variables");
    CHECK(bv && bv->elements() && bv->elements()->size() == 4);   // the SC7852's four pages
    // writeMemory, readMemory, the memory event.
    c.request("writeMemory", obj({{"memoryReference", Value("0xC100")}, {"data", Value(cu1600::dap::base64("\x5A"))}}));
    CHECK(c.event("memory", 0).has_value());
    const Value mem = c.request("readMemory", obj({{"memoryReference", Value("0xC100")}, {"count", Value(1)}}));
    CHECK_EQ(cu1600::dap::unbase64(cu1600::dap::textOf(mem, "data")), std::string("\x5A"));
    const Value dis = c.request("disassemble", obj({{"memoryReference", Value("0x0000")}, {"instructionCount", Value(4)}}));
    const Value* ins = dis.find("instructions");
    CHECK(ins && ins->elements() && ins->elements()->size() == 4);
    CHECK_EQ(cu1600::dap::textOf(c.request("evaluate", obj({{"expression", Value("2+3*4")}})), "result"), std::string("0x0E (14)"));
    // An error names what is wrong.
    CHECK(c.refusal("evaluate", obj({{"expression", Value("nosuch + 1")}})).find("nosuch") != std::string::npos);
    // Only the registers of now can be changed.
    c.request("stepIn", obj({{"threadId", Value(1)}, {"granularity", Value("instruction")}}));
    CHECK(c.event("stopped").has_value());
    CHECK(c.refusal("setVariable", obj({{"variablesReference", Value(1001 * 4 + 1)}, {"name", Value("A")}, {"value", Value("1")}}))
              .find("live frame") != std::string::npos);
    CHECK(c.refusal("setVariable", obj({{"variablesReference", Value(1000 * 4 + 1)}, {"name", Value("A")}, {"value", Value("1")}}))
              .empty());
    CHECK_EQ(c.refusal("frobnicate"), std::string("Unsupported request 'frobnicate'"));
    (void)scopes;
}

// ── Events ───────────────────────────────────────────────────────────────

TEST(stopped_once_with_reason_ids_and_pause_from_elsewhere) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.begin();
    c.request("attach");
    c.request("configurationDone");
    fx.runFor(0.2);
    // A pause from elsewhere (the harness, a preset): told once.
    fx.part().debuggerPart().runControl().pause(calcu::debugger::kCpu);
    const auto stop = c.event("stopped");
    CHECK(stop && cu1600::dap::textOf(*stop, "reason") == "pause");
    fx.runFor(0.2);
    CHECK_EQ(c.count("stopped"), 0);   // once
    const Value st = c.request("stackTrace", obj({{"threadId", Value(1)}, {"levels", Value(1)}}));
    const std::string pc = cu1600::dap::textOf(st.find("stackFrames")->elements()->front(), "instructionPointerReference");
    const Value bps = c.request("setInstructionBreakpoints",
                                obj({{"breakpoints", Value(Value::Array{obj({{"instructionReference", Value(pc)}})})}}));
    const std::int64_t id = cu1600::dap::intOf(bps.find("breakpoints")->elements()->front(), "id");
    c.request("continue", obj({{"threadId", Value(1)}}));
    const auto hit = c.event("stopped");
    CHECK(hit && cu1600::dap::textOf(*hit, "reason") == "breakpoint" && cu1600::dap::boolOf(*hit, "allThreadsStopped"));
    const Value* ids = hit ? hit->find("hitBreakpointIds") : nullptr;
    CHECK(ids && ids->elements() && cu1600::dap::intOf(Value::object({{"x", ids->elements()->front()}}), "x") == id);
}

// ── The server's own requests ────────────────────────────────────────────

TEST(reset_display_text_quit) {
    SKIP_UNLESS(sessionfixture::romsPresent(), "no ROM set");
    SessionFixture fx;
    CHECK(started(fx));
    Client c(fx);
    c.connect();
    fx.runFor(0.5);
    // c1600/display-text without a DAP session (no initialize): the keys and
    // value forms of the harness's `display-text` (on, rows, indicators, notText).
    const Value text = c.request("c1600/display-text");
    const Value* rows = text.find("rows");
    CHECK(rows && rows->elements() && !rows->elements()->empty() && cu1600::dap::boolOf(text, "on"));
    CHECK(cu1600::dap::isArray(text.find("indicators")));
    CHECK(cu1600::dap::intOf(text, "notText", -1) >= 0);
    CHECK(!text.find("poweredOn") && !text.find("unparsed") && !text.find("status"));
    CHECK_EQ(c.refusal("c1600/screen"), std::string("Unsupported request 'c1600/screen'"));   // `c1600/screen` is no request
    c.request("initialize");
    c.request("attach");
    c.request("configurationDone");
    c.request("c1600/reset", obj({{"kind", Value("reset")}}));
    auto stop = c.event("stopped");
    CHECK(stop && cu1600::dap::textOf(*stop, "reason") == "entry" && c.rc().paused());
    c.request("c1600/reset", obj({{"kind", Value("allReset")}, {"stop", Value(false)}}));
    fx.call();
    CHECK(!c.rc().paused());   // `stop: false` runs on
    c.request("c1600/quit");
    CHECK_EQ(c.t.quits, 1);    // the program ends
    CHECK(!fx.part().dapPart()->hasClient());
}

} // namespace

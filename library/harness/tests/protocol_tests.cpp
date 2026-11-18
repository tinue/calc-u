// Replies, lines, the one-shot form, the registry.
#include "TestHarness.hpp"
#include "TestProduct.hpp"
#include "harness/Commands.hpp"
#include "harness/Protocol.hpp"
#include "harness/Reply.hpp"
#include <string>
#include <vector>

using namespace calcu::harness;
using namespace calcu::harness::test;

namespace {
CommandRegistry registryOf(TestProduct& p) {
    CommandRegistry r;
    addFirstCommands(r);
    p.addCommands(r);
    return r;
}
const CommandLine* lineOf(const std::variant<CommandLine, std::string>& v) { return std::get_if<CommandLine>(&v); }
} // namespace

// ── Replies ─────────────────────────────────────────────────────────────

TEST(plain_reply_is_lines) {
    CHECK_EQ(writePlain(doneReply()), std::string("done\n"));
    CHECK_EQ(writePlain(doneReply(Value(1), {"a", "b"})), std::string("done 2\na\nb\n"));
    CHECK_EQ(writePlain(refusedReply("Not now.")), std::string("refused: Not now.\n"));
    // A line break in a result line or a reason does not end the reply early.
    CHECK_EQ(writePlain(doneReply({}, {"x\ny"})), std::string("done 1\nx y\n"));
    CHECK_EQ(writePlain(refusedReply("a\nb")), std::string("refused: a b\n"));
}

TEST(json_reply_has_the_three_keys_for_every_command) {
    CHECK_EQ(writeJson(doneReply()), std::string("{\"done\":true,\"result\":null,\"reason\":null}\n"));
    CHECK_EQ(writeJson(refusedReply("No.")), std::string("{\"done\":false,\"result\":null,\"reason\":\"No.\"}\n"));
    Value v = Value::object();
    v.set("s", Decimal{1500, 3}).set("t", "a\"b\\c\x01");
    CHECK_EQ(writeJson(doneReply(v)),
             std::string("{\"done\":true,\"result\":{\"s\":1.500,\"t\":\"a\\\"b\\\\c\\u0001\"},\"reason\":null}\n"));
    // Every registered command, done and refused, in one run with --json.
    RunOptions o;
    o.json = true;
    TestRun run(o);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const std::vector<std::string> lines = {
        "wait 1ms", "wait", "key-down 1", "key-up 1", "key-down nokey", "display-text", "display", "state busy", "state nope",
        "capabilities", "ticks", "audio x.wav", "state-save", "state-restore /no/such/file", "test mapping { a: 1 }",
        "test operation 10", "test fail", "test count", "nothing"};
    for (const auto& l : lines) {
        const std::string r = run.line(l);
        CHECK(r.rfind("{\"done\":", 0) == 0);
        CHECK(r.find(",\"result\":") != std::string::npos && r.find(",\"reason\":") != std::string::npos);
        CHECK(!r.empty() && r.back() == '\n' && r.find('\n') == r.size() - 1);   // one line
    }
}

TEST(refusal_carries_the_reason) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("key-down nokey"), std::string("refused: Unknown key: nokey.\n"));
    CHECK(run.line("state nope").rfind("refused: There is no topic nope.", 0) == 0);
    CHECK_EQ(run.line("audio sound.wav"), std::string("refused: This machine has no sound.\n"));
}

// ── Lines and the registry ──────────────────────────────────────────────

TEST(commands_part_on_then) {
    const std::vector<std::string> words = {"wait", "2s", "then", "display-text", "then", "test", "mapping", "{", "a:", "1", "}"};
    const auto lines = splitOneShot(words);
    CHECK_EQ(lines.size(), std::size_t{3});
    if (lines.size() == 3) {
        CHECK_EQ(lines[0], std::string("wait 2s"));
        CHECK_EQ(lines[1], std::string("display-text"));
        CHECK_EQ(lines[2], std::string("test mapping { a: 1 }"));
    }
    const std::vector<std::string> one = {"ticks"};
    CHECK(splitOneShot(one) == std::vector<std::string>{"ticks"});
}

TEST(line_that_is_no_command_is_refused) {
    TestProduct p;
    const CommandRegistry r = registryOf(p);
    auto v = parseLine("jump 3", r);
    CHECK(std::holds_alternative<std::string>(v) && std::get<std::string>(v) == "unknown command jump");
    v = parseLine("test nothing", r);
    CHECK(std::holds_alternative<std::string>(v));
    // An empty line, or one of blanks and \r, is no command and no refusal.
    v = parseLine(" \r", r);
    CHECK(lineOf(v) && lineOf(v)->path.empty());
    // serve goes on after a refusal.
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    std::vector<std::string> in = {"jump", "ticks", "quit", "ticks"}, out;
    std::size_t at = 0;
    const int status = run.server->serve([&]() -> std::optional<std::string> {
        if (at == in.size()) return std::nullopt;
        return in[at++];
    }, [&](const std::string& s) { out.push_back(s); });
    CHECK_EQ(status, 0);
    CHECK_EQ(out.size(), std::size_t{2});
    if (out.size() == 2) {
        CHECK_EQ(out[0], std::string("refused: unknown command jump\n"));
        CHECK(out[1].rfind("done 2\nticks: ", 0) == 0);
    }
}

TEST(product_command_under_product_word) {
    TestProduct p;
    const CommandRegistry r = registryOf(p);
    auto v = parseLine("test count", r);
    CHECK(lineOf(v) && lineOf(v)->path == (std::vector<std::string>{"test", "count"}));
    CHECK(r.find({"test", "count"}) != nullptr);
    CHECK(r.find({"count"}) == nullptr);   // not at the top level
    CHECK(std::holds_alternative<std::string>(parseLine("count", r)));
}

TEST(common_command_at_top_level) {
    TestProduct p;
    const CommandRegistry r = registryOf(p);
    for (const char* c : {"wait", "key-down", "key-up", "display-text", "display", "state", "capabilities", "ticks", "audio",
                          "state-save", "state-restore", "quit"}) {
        CHECK(r.find({c}) != nullptr);
        CHECK(r.find({"test", c}) == nullptr);
    }
    // The product-specific part of a common command comes from the product:
    // `display-text` of the test product reads its segments by the product's rule.
    TestRun run;
    CHECK(run.server && run.line("display-text").rfind("done 2\n010\n", 0) == 0);   // 1667 ticks after the start
}

TEST(step_becomes_command_by_dropping_dash_and_colon) {
    TestProduct p;
    const CommandRegistry r = registryOf(p);
    // The step `- test mapping: { a: 1, b: x }` becomes this line.
    auto v = parseLine("test mapping { a: 1, b: x }", r);
    CHECK(lineOf(v) && lineOf(v)->argText == "{ a: 1, b: x }");
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("test mapping { a: 1, b: x }"), std::string("done 1\n{\"a\":\"1\",\"b\":\"x\"}\n"));
    CHECK_EQ(run.line("test mapping { place: slot-1, as: live, name: My card }"),
             std::string("done 1\n{\"place\":\"slot-1\",\"as\":\"live\",\"name\":\"My card\"}\n"));
    CHECK(run.line("test mapping { a: 1").rfind("refused: ", 0) == 0);
}

// Copy and Cut in the session, with a test frontend that offers the clipboard.
#include "PaperTestKit.hpp"
#include "TestFrontend.hpp"
#include "TestHarness.hpp"
#include "paper/Picture.hpp"
#include "paper/PaperTopics.hpp"
#include "sessions/AppSession.hpp"

using namespace calcu::paper;
using namespace calcu::paper::test;
namespace s = calcu::sessions;

namespace {

struct ClipRun {
    PaperRun run;
    s::test::TestFrontend fe;
    s::ListenerId id = 0;
    explicit ClipRun(bool offer = true) {
        if (!run.server) return;
        s::HostServices services;
        if (offer) services.offered = {s::HostService::ClipboardImage, s::HostService::ClipboardText};
        id = session().listen(fe, services);
    }
    s::AppSession& session() { return run.server->host().session(); }
    s::CallId send(const s::CommandName& c, s::Value args) { return session().send(id, s::Command{c, std::move(args)}); }
    // The asks for the clipboard from `mark` on.
    std::vector<s::ServiceAsk> asks(std::size_t mark = 0) const {
        std::vector<s::ServiceAsk> out;
        for (const auto& m : fe.since(mark))
            if (const auto* a = std::get_if<s::ServiceAsk>(&m); a && a->service == s::HostService::ClipboardImage)
                out.push_back(*a);
        return out;
    }
    std::vector<const CopyOutcome*> outcomes() {
        std::vector<const CopyOutcome*> out;
        for (const auto& p : fe.mirror.takeAppended(kPaperCopiesTopic))
            if (const auto* o = s::payloadAs<CopyOutcome>(p)) out.push_back(o);
        return out;
    }
    std::vector<s::PayloadPtr> kept;
};

s::Value owner(const char* o) { return s::Value::object({{"owner", o}}); }

} // namespace

TEST(copy_asks_clipboard_with_picture_and_text) {
    ClipRun c;
    if (!c.run.server) return;
    c.run.line("attach printer test-dotter");
    c.run.line("attach aux test-plotter");
    c.run.online();
    c.run.press("1");
    c.run.press("2");
    // TD-20: the picture and the text, the lines joined by line ends.
    const s::CallId copy = c.send(kPaperCopyCommand, owner("td-20"));   // any case
    CHECK(c.fe.reply(copy) && c.fe.reply(copy)->done);
    auto asks = c.asks();
    CHECK_EQ(asks.size(), std::size_t(1));
    if (asks.size() != 1) return;
    const s::Value& a = asks[0].args;
    const auto png = picturePng(c.run.paperOf("TD-20"));
    CHECK(a.find("bytes") && a.find("bytes")->bytes() && *a.find("bytes")->bytes() == png);
    CHECK(a.find("text") && *a.find("text")->string() == std::string("KEY 1   \nKEY 2   "));
    CHECK(a.find("name") && *a.find("name")->string() == std::string("TD-20.png"));
    CHECK(a.find("dotsPerInch") && a.find("dotsPerInch")->real() && *a.find("dotsPerInch")->real() == 300.0);
    // TP-4: the picture only.
    const std::size_t mark = c.fe.log.size();
    c.send(kPaperCopyCommand, owner("TP-4"));
    asks = c.asks(mark);
    CHECK_EQ(asks.size(), std::size_t(1));
    if (asks.size() == 1) {
        CHECK(asks[0].args.find("bytes") != nullptr);
        CHECK(asks[0].args.find("text") == nullptr);
        CHECK_EQ(*asks[0].args.find("name")->string(), std::string("TP-4.png"));
    }
}

TEST(cut_copies_then_cuts_also_when_copy_failed) {
    ClipRun c;
    if (!c.run.server) return;
    c.run.line("attach printer test-dotter");
    c.run.online();
    c.run.press("1");
    const s::CallId cut = c.send(kPaperCutCommand, s::Value::object({{"copy", true}}));   // the printer plugged in
    CHECK(c.fe.reply(cut) && c.fe.reply(cut)->done);
    CHECK_EQ(c.asks().size(), std::size_t(1));       // copied first
    CHECK(c.run.paperOf("TD-20").empty());          // then cut, before any answer
    const auto asks = c.asks();
    if (!asks.empty()) c.session().answer(asks[0].ask, s::ServiceAnswer{false, {}, "refused"});
    CHECK(c.run.paperOf("TD-20").empty());
    // Nobody offers the clipboard: the cut is done all the same; a copy is refused.
    ClipRun none(false);
    if (!none.run.server) return;
    none.run.line("attach printer test-dotter");
    none.run.online();
    none.run.press("1");
    const s::CallId copy = none.send(kPaperCopyCommand, s::Value());
    CHECK(none.fe.reply(copy) && !none.fe.reply(copy)->done && none.fe.reply(copy)->reason == kNoClipboard);
    const s::CallId cut2 = none.send(kPaperCutCommand, s::Value::object({{"copy", true}}));
    CHECK(none.fe.reply(cut2) && none.fe.reply(cut2)->done);
    CHECK(none.run.paperOf("TD-20").empty());
}

TEST(empty_paper_copy_and_cut_refused) {
    ClipRun c;
    if (!c.run.server) return;
    c.run.line("attach printer test-dotter");
    const s::CallId copy = c.send(kPaperCopyCommand, s::Value());
    const s::CallId cut = c.send(kPaperCutCommand, s::Value::object({{"copy", true}}));
    CHECK(c.fe.reply(copy) && !c.fe.reply(copy)->done && c.fe.reply(copy)->reason == kPaperEmpty);
    CHECK(c.fe.reply(cut) && !c.fe.reply(cut)->done && c.fe.reply(cut)->reason == kPaperEmpty);
    CHECK(c.asks().empty());
    // `cut-paper` (no copy) on an empty paper is done.
    const s::CallId plain = c.send(kPaperCutCommand, s::Value::object({{"copy", false}}));
    CHECK(c.fe.reply(plain) && c.fe.reply(plain)->done);
}

TEST(copy_holds_marks_up_to_the_command) {
    ClipRun c;
    if (!c.run.server) return;
    c.run.line("attach printer test-dotter");
    c.run.online();
    c.run.line("key-down 1");
    c.run.server->host().advance(45);   // two rows of the line printed
    const auto upToNow = picturePng(c.run.paperOf("TD-20"));
    c.send(kPaperCopyCommand, s::Value());
    c.run.line("wait 100ms");   // the rest of the line after the copy
    const auto asks = c.asks();
    CHECK_EQ(asks.size(), std::size_t(1));
    if (asks.empty()) return;
    CHECK(*asks[0].args.find("bytes")->bytes() == upToNow);
    CHECK(picturePng(c.run.paperOf("TD-20")) != upToNow);
}

TEST(copy_outcome_on_paper_copies) {
    ClipRun c;
    if (!c.run.server) return;
    c.run.line("attach printer test-dotter");
    c.run.online();
    c.run.press("1");
    for (int i = 0; i < 3; ++i) c.send(kPaperCopyCommand, s::Value());
    const auto asks = c.asks();
    CHECK_EQ(asks.size(), std::size_t(3));
    if (asks.size() != 3) return;
    c.session().answer(asks[0].ask, s::ServiceAnswer{true, {}, ""});
    c.session().answer(asks[1].ask, s::ServiceAnswer{true, s::Value::object({{"textOnly", true}}), ""});
    c.session().answer(asks[2].ask, s::ServiceAnswer{false, {}, "The browser took nothing."});
    const auto o = c.outcomes();
    CHECK_EQ(o.size(), std::size_t(3));
    if (o.size() != 3) return;
    CHECK(o[0]->owner == "TD-20" && o[0]->outcome == CopyOutcome::Kind::Copied);
    CHECK(o[1]->outcome == CopyOutcome::Kind::TextOnly);
    CHECK(o[2]->outcome == CopyOutcome::Kind::Failed && o[2]->reason == "The browser took nothing.");
    CHECK_EQ(toText(o[1]->toValue()), std::string("{\"owner\":\"TD-20\",\"outcome\":\"text-only\",\"reason\":\"\"}"));
}

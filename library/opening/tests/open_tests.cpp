// The open part: the request, the reply, a medium, the file kept for later,
// several files; over the real presets and media parts (OpenRig.hpp).
#include "OpenRig.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "sessions/LibraryTopics.hpp"
#include <algorithm>

using namespace calcu::opening;
using namespace calcu::opening::test;

namespace {
bool isAs(const OpenReply* o, OpenedAs as, const std::string& detail = "") {
    return o && o->as == as && (detail.empty() || o->detail == detail);
}
OpenRigOptions withUploads(const calcu::testing::TempFolder& up, std::string model = "test-seg") {
    OpenRigOptions o;
    o.model = std::move(model);
    o.adjust = [folder = up.str()](OpenSetup& s) { s.uploadFolder = folder; };
    return o;
}
} // namespace

// ── The request ──

TEST(open_request_value_round_trip) {
    OpenRequest r;
    r.files = {OpenedFile{"a.tseg", "/x/a.tseg", bytesOf("model: x\n")}, OpenedFile{"b.lst", std::nullopt, std::nullopt}};
    r.way = Way::OpenWith;
    r.upload = true;
    r.confirmed = true;
    std::string why;
    const auto back = OpenRequest::fromValue(r.toValue(), &why);
    CHECK(back.has_value());
    if (!back) return;
    CHECK(back->files.size() == 2 && back->files[0].name == "a.tseg" && back->files[0].path == std::optional<std::string>("/x/a.tseg"));
    CHECK(back->files[0].bytes == r.files[0].bytes);
    CHECK(!back->files[1].path && !back->files[1].bytes);
    CHECK(back->way == Way::OpenWith && back->upload && back->confirmed);
    for (Way w : {Way::Drop, Way::OpenWith, Way::Start, Way::Dialog, Way::Harness}) {
        const auto again = OpenRequest::fromValue(OpenRequest{r.files, w, false, false}.toValue(), &why);
        CHECK(again && again->way == w);
    }
    CHECK(!OpenRequest::fromValue(Value::object({{"way", "drop"}}), &why) && !why.empty());   // no files
    CHECK(!OpenRequest::fromValue(Value::object({{"files", Value(Value::Array{Value::object({{"name", "a"}})})}, {"way", "fly"}}), &why));
    const OpenReply o(OpenedAs::Medium, "w.tdisk.yaml", "drive");
    CHECK(o.toValue() == Value::object({{"as", "medium"}, {"file", "w.tdisk.yaml"}, {"detail", "drive"}}));
}

// ── The reply ──

TEST(open_reply_says_preset_medium_kind) {
    calcu::testing::TempFolder dir("calcu-open");
    dir.write("game.test-seg", header("test-dot"));
    OpenRig r;
    const auto* p = r.opened(r.onDisk(dir, "game.test-seg"));
    CHECK(p && p->done && isAs(r.replyOf(p), OpenedAs::Preset));
    CHECK(r.replyOf(p) && r.replyOf(p)->file == "game.test-seg");
    CHECK_EQ(r.model(), std::string("test-dot"));
    const auto* up = r.opened(file("game.test-seg", header("test-seg"), "/uploads/game.test-seg"), Way::Drop, true);
    CHECK(up && up->done && r.model() == "test-seg");   // an upload: as its content
    const auto* nopath = r.opened(file("other.test-seg", header("test-dot")));
    CHECK(nopath && nopath->done && r.model() == "test-dot");   // no path: as its content
    dir.write("w.testdisk.yaml", disk("W", "content").text());
    const auto* m = r.opened(r.onDisk(dir, "w.testdisk.yaml"));
    CHECK(m && m->done && isAs(r.replyOf(m), OpenedAs::Medium, "disk"));
    const auto* k = r.opened(file("p.lst", "10 PRINT"));
    CHECK(k && k->done && isAs(r.replyOf(k), OpenedAs::FileKind, "test.listing"));
    const auto* b = r.opened(file("x.bin", "BLOB"));   // a kind that starts an operation: replied at its end
    CHECK(b && b->done && isAs(r.replyOf(b), OpenedAs::FileKind, "test.blob"));
}

TEST(open_reply_says_kept) {
    OpenRig r;
    r.runOperation(200'000);
    const CallId c = r.open(file("p.lst", "x"));
    const auto* k = r.fixture.fe.reply(c);   // at once
    CHECK(k && k->done && isAs(r.replyOf(k), OpenedAs::Kept));
    CHECK(r.replyOf(k) && r.replyOf(k)->toValue().find("as")->string() && *r.replyOf(k)->toValue().find("as")->string() == "kept");
}

TEST(open_reply_says_why_refused) {
    OpenRig r;
    const auto* a = r.opened(file("photo.jpg", "x"));
    CHECK(a && !a->done && a->reason == "Test-Product cannot open photo.jpg." && !a->result);
    const auto* b = r.opened(file("x.bin", "BLOB refuse"));   // a kind's own refusal, with the notice added
    CHECK(b && !b->done && b->reason == "x.bin is a blob this product does not take.");
    CHECK(hasText(r.notices(), "x.bin is a blob this product does not take."));
}

// ── A medium ──

TEST(open_medium_checked_first_refusal_passed_on) {
    OpenRig r("test-dot");
    TestFile bad = disk("Old", "o");
    bad.version = 1;
    const auto* a = r.opened(file("bad.testdisk.yaml", bad.text(), "/m/bad.testdisk.yaml"));
    CHECK(a && !a->done && a->reason.find("cannot be read: Its version is not known.") != std::string::npos);
    CHECK(hasText(r.notices(), "cannot be read: Its version is not known."));
    CHECK(!r.fixture.place("disk")->holds);
    const auto* up = r.opened(file("bad.testdisk.yaml", bad.text()), Way::Drop, true);   // an upload: not imported either
    CHECK(up && !up->done && !r.fixture.exists("bad.testdisk.yaml"));
}

TEST(open_medium_used_where_it_lies) {
    OpenRig r("test-dot");
    calcu::testing::TempFolder outside("calcu-open-outside");
    outside.write("w.testdisk.yaml", disk("W", "content").text());
    const auto* a = r.opened(r.onDisk(outside, "w.testdisk.yaml"));
    CHECK(a && a->done && isAs(r.replyOf(a), OpenedAs::Medium, "disk"));
    CHECK_EQ(r.fixture.drive()->bytes(), std::string("content"));
    // The file is the instance: written back where it lies, nothing copied.
    r.fixture.drive()->write("written back");
    r.fixture.ok("media.refresh");
    CHECK_EQ(field(calcu::testing::readText(outside.path() / "w.testdisk.yaml"), "content"), std::string("written back"));
    CHECK(!r.fixture.exists("w.testdisk.yaml"));
}

TEST(open_template_not_written) {
    OpenRig r("test-dot");
    calcu::testing::TempFolder outside("calcu-open-outside");
    outside.write("blank.testdisk.yaml", disk("Blank", "b", true).text());
    CHECK(r.opened(r.onDisk(outside, "blank.testdisk.yaml"))->done);
    r.fixture.drive()->write("changed");
    r.fixture.ok("media.refresh");
    CHECK_EQ(field(calcu::testing::readText(outside.path() / "blank.testdisk.yaml"), "content"), std::string("b"));
}

TEST(open_holding_place_inserted_after_eject) {
    OpenRig r("test-dot");
    calcu::testing::TempFolder outside("calcu-open-outside");
    outside.write("one.testdisk.yaml", disk("One", "1").text());
    outside.write("two.testdisk.yaml", disk("Two", "2").text());
    CHECK(r.opened(r.onDisk(outside, "one.testdisk.yaml"))->done);
    CHECK(r.opened(r.onDisk(outside, "two.testdisk.yaml"))->done);
    CHECK_EQ(r.fixture.drive()->bytes(), std::string("2"));
    CHECK(r.fixture.place("disk")->holds && r.fixture.place("disk")->holds->name == "Two");
}

TEST(open_module_one_slot_plugged) {
    OpenRig r("test-seg");
    calcu::testing::TempFolder outside("calcu-open-outside");
    outside.write("m.testmodule.yaml", moduleFile("Bank", "bank").text());
    const CallId c = r.open(r.onDisk(outside, "m.testmodule.yaml"));
    CHECK(r.fixture.session->busy());   // the attach of the slot's picker is an operation
    r.fixture.callWhileBusy();
    const auto* a = r.fixture.fe.reply(c);
    CHECK(a && a->done && isAs(r.replyOf(a), OpenedAs::Medium, "slot-1"));
    CHECK(r.fixture.module("slot-1") != nullptr);
}

TEST(open_module_two_slots_refused) {
    OpenRig r("test-dot");
    calcu::testing::TempFolder outside("calcu-open-outside");
    outside.write("m.testmodule.yaml", moduleFile("Bank", "bank").text());
    const auto* a = r.opened(r.onDisk(outside, "m.testmodule.yaml"));
    CHECK(a && !a->done && a->reason == "m.testmodule.yaml fits more than one place; choose it in the picker of the place it should go into.");
    CHECK(hasText(r.notices(), "fits more than one place"));
    CHECK(!r.fixture.module("card-1") && !r.fixture.module("card-2"));
}

TEST(open_no_place_now_refused_says_where) {
    OpenRig r("test-dot");
    const auto* a = r.opened(file("c.testcard.yaml", card("C", "c").text(), "/m/c.testcard.yaml"));
    CHECK(a && !a->done);
    CHECK(a && a->reason.rfind("The test-dot has no place for c.testcard.yaml now; a test card goes into ", 0) == 0);
    CHECK(!r.fixture.exists("c.testcard.yaml"));   // not copied
}

TEST(open_upload_imported_then_placed) {
    OpenRig r("test-dot");
    const auto* a = r.opened(file("w.testdisk.yaml", disk("W", "up").text(), "/uploads/w.testdisk.yaml"), Way::Drop, true);
    CHECK(a && a->done && isAs(r.replyOf(a), OpenedAs::Medium, "disk"));
    CHECK(r.fixture.exists("W.testdisk.yaml"));   // saved under the medium's name
    CHECK_EQ(r.fixture.drive()->bytes(), std::string("up"));
}

TEST(open_upload_name_taken_asks_then_replace) {
    OpenRigOptions o;
    o.model = "test-dot";
    o.prepare = [](auto& save, auto&) { save.write("w.testdisk.yaml", disk("W", "old").text()); };
    OpenRig r(o);
    const auto* a = r.opened(file("w.testdisk.yaml", disk("W", "new").text()), Way::Drop, true);
    CHECK(a && !a->done && a->reason.rfind("asks: ", 0) == 0);
    CHECK(r.notices().empty());   // a question, not a notice
    CHECK(!r.fixture.place("disk")->holds && field(r.fixture.saved("w.testdisk.yaml"), "content") == "old");
    const auto* yes = r.opened(file("w.testdisk.yaml", disk("W", "new").text()), Way::Drop, true, true);
    CHECK(yes && yes->done && r.fixture.drive()->bytes() == "new");
    CHECK_EQ(field(r.fixture.saved("w.testdisk.yaml"), "content"), std::string("new"));
}

TEST(no_place_refused_in_place_imported_as_upload) {
    OpenRig r("test-seg");   // no drive
    const auto* a = r.opened(file("w.testdisk.yaml", disk("W", "x").text(), "/m/w.testdisk.yaml"));
    CHECK(a && !a->done && a->reason.find("has no place for w.testdisk.yaml now") != std::string::npos);
    CHECK(!r.fixture.exists("w.testdisk.yaml"));   // not copied
    const auto* up = r.opened(file("w.testdisk.yaml", disk("W", "x").text()), Way::Drop, true);
    CHECK(up && up->done && isAs(r.replyOf(up), OpenedAs::Medium, "imported"));
    CHECK(r.fixture.exists("W.testdisk.yaml"));   // saved under the medium's name
    CHECK(hasText(r.notices(), "has no place for w.testdisk.yaml now"));   // in the save folder, and told so
}

TEST(place_passing_medium_refuses_nothing_imported) {
    OpenRig r("test-seg");
    calcu::testing::TempFolder outside("calcu-open-outside");
    outside.write("a.testcard.yaml", card("A", "a").text());
    CHECK(r.opened(r.onDisk(outside, "a.testcard.yaml"))->done);
    CHECK(r.fixture.ok("media.feed", Value::object({{"place", "reader"}, {"side", 0}})));
    const auto* a = r.opened(file("b.testcard.yaml", card("B", "b").text()), Way::Drop, true);
    CHECK(a && !a->done && a->reason == "A test card is passing through reader now.");
    CHECK(hasText(r.notices(), "passing through reader"));
    CHECK(!r.fixture.exists("b.testcard.yaml"));
}

TEST(no_answer_nothing_imported_nothing_placed) {
    OpenRigOptions o;
    o.model = "test-dot";
    o.prepare = [](auto& save, auto&) { save.write("w.testdisk.yaml", disk("W", "old").text()); };
    OpenRig r(o);
    const auto* a = r.opened(file("w.testdisk.yaml", disk("W", "new").text()), Way::Drop, true);
    CHECK(a && a->reason.rfind("asks: ", 0) == 0);
    // The user says no: the frontend sends nothing more.
    r.fixture.call();
    CHECK(!r.fixture.place("disk")->holds);
    CHECK_EQ(field(r.fixture.saved("w.testdisk.yaml"), "content"), std::string("old"));
}

// ── Kept for later ──

TEST(kept_while_operation_runs_opened_when_free) {
    OpenRig r;
    const CallId op = r.runOperation(200'000);
    const CallId c = r.open(file("x.bin", "BLOB"));
    CHECK(isAs(r.replyOf(r.fixture.fe.reply(c)), OpenedAs::Kept));
    CHECK(r.blob.opened.empty());
    r.fixture.callWhileBusy();   // the operation ends, then the kept file starts its own
    CHECK(r.fixture.fe.reply(op) && r.fixture.fe.reply(op)->done);
    if (r.blob.opened.empty()) r.fixture.call();
    r.fixture.callWhileBusy();
    CHECK(r.blob.opened == std::vector<std::string>{"x.bin"});
    CHECK_EQ(r.fixture.fe.replies(c), 1);   // replied once, with Kept
    CHECK(!r.fixture.session->busy());
}

TEST(kept_before_machine_built) {
    OpenRigOptions o;
    o.start = false;   // no machine yet: the start has not finished
    OpenRig r(o);
    const CallId c = r.open(file("p.lst", "x"));
    CHECK(isAs(r.replyOf(r.fixture.fe.reply(c)), OpenedAs::Kept));
    r.fixture.call();
    CHECK(r.listing.opened.empty());
    r.fixture.send("start", Value::object({{"model", "test-seg"}}));
    r.fixture.callWhileBusy();
    r.fixture.call();
    CHECK(r.listing.opened == std::vector<std::string>{"p.lst"});
    r.fixture.call();
    CHECK(r.listing.opened.size() == 1);   // once
}

TEST(second_kept_file_replaces_first) {
    OpenRig r;
    r.runOperation(200'000);
    r.fixture.logLines();
    r.open(file("first.lst", "x"));
    const CallId second = r.open(file("second.lst", "x"));
    CHECK(isAs(r.replyOf(r.fixture.fe.reply(second)), OpenedAs::Kept));
    CHECK(hasText(r.fixture.logLines(), "first.lst"));   // a log line, no notice
    CHECK(r.notices().empty());
    r.fixture.callWhileBusy();
    r.fixture.call();
    CHECK(r.listing.opened == std::vector<std::string>{"second.lst"});
}

TEST(kept_file_refused_later_is_a_notice) {
    OpenRig r;
    r.runOperation(200'000);
    const CallId c = r.open(file("photo.jpg", "x"));
    r.fixture.callWhileBusy();
    r.fixture.call();
    CHECK(hasText(r.notices(), "Test-Product cannot open photo.jpg."));
    CHECK_EQ(r.fixture.fe.replies(c), 1);   // no second reply
}

// ── Several files ──

TEST(several_files_one_folder_preset_found_by_path) {
    calcu::testing::TempFolder up("calcu-uploads");
    OpenRig r(withUploads(up));
    OpenRequest q;
    q.files = {file("data.testdisk.yaml", disk("Data", "d").text()),
               file("game.test-seg", keys("test-dot", "  - test-disk: { file: data.testdisk.yaml }\n"))};
    q.way = Way::Dialog;
    q.upload = true;
    const CallId c = r.open(q);
    r.fixture.callWhileBusy();
    const auto* reply = r.fixture.fe.reply(c);
    CHECK(reply && reply->done && isAs(r.replyOf(reply), OpenedAs::Preset));
    CHECK_EQ(r.fixture.drive() ? r.fixture.drive()->bytes() : std::string(), std::string("d"));   // the file it names beside it
    const std::filesystem::path folder = up.path() / "open-1";
    CHECK(std::filesystem::exists(folder / "game.test-seg") && std::filesystem::exists(folder / "data.testdisk.yaml"));
}

TEST(several_files_named_file_found_by_its_name_alone) {
    calcu::testing::TempFolder up("calcu-uploads");
    OpenRig r(withUploads(up));
    OpenRequest q;
    q.files = {file("data.testdisk.yaml", disk("Data", "d").text()),
               file("game.test-seg", keys("test-dot", "  - test-disk: { file: disks/data.testdisk.yaml }\n"))};
    q.way = Way::Dialog;
    q.upload = true;
    const CallId c = r.open(q);
    r.fixture.callWhileBusy();
    const auto* reply = r.fixture.fe.reply(c);
    CHECK(reply && reply->done);
    CHECK_EQ(r.fixture.drive() ? r.fixture.drive()->bytes() : std::string(), std::string("d"));
}

TEST(uploaded_preset_without_a_file_it_names_refused_with_the_advice) {
    calcu::testing::TempFolder up("calcu-uploads");
    OpenRig r(withUploads(up));
    const std::string advice =
        "The preset game.test-seg names the file data.testdisk.yaml, which was not chosen with it: "
        "choose the preset together with every file it names.";
    // Alone: a folder of its own, the same advice.
    OpenRequest q;
    q.files = {file("game.test-seg", keys("test-dot", "  - test-disk: { file: data.testdisk.yaml }\n"))};
    q.way = Way::Dialog;
    q.upload = true;
    CallId c = r.open(q);
    r.fixture.callWhileBusy();
    const auto* reply = r.fixture.fe.reply(c);
    CHECK(reply && !reply->done && reply->reason.find(advice) != std::string::npos);
    CHECK(hasText(r.notices(), advice));
    CHECK(std::filesystem::exists(up.path() / "open-1" / "game.test-seg"));
    // With another file, not the one it names.
    q.files.push_back(file("other.testdisk.yaml", disk("Other", "o").text()));
    c = r.open(q);
    r.fixture.callWhileBusy();
    reply = r.fixture.fe.reply(c);
    CHECK(reply && !reply->done && reply->reason.find(advice) != std::string::npos);
}

TEST(several_files_first_preset_by_name) {
    calcu::testing::TempFolder up("calcu-uploads");
    OpenRig r(withUploads(up));
    OpenRequest q;
    q.files = {file("b.test-seg", header("test-seg")), file("notes.txt", header("test-seg")), file("a.test-dot", header("test-dot"))};
    q.way = Way::Dialog;
    q.upload = true;
    r.open(q);
    r.fixture.callWhileBusy();
    CHECK_EQ(r.model(), std::string("test-dot"));   // a.test-dot
    // None with a preset extension: the first by name of those with a model line.
    q.files = {file("z.txt", header("test-dot")), file("m.txt", header("test-seg")), file("x.lst", "x")};
    r.open(q);
    r.fixture.callWhileBusy();
    CHECK_EQ(r.model(), std::string("test-seg"));   // m.txt
    CHECK(r.listing.opened.empty());
}

TEST(several_files_no_preset_first_file_alone) {
    calcu::testing::TempFolder up("calcu-uploads");
    OpenRig r(withUploads(up));
    OpenRequest q;
    q.files = {file("q.lst", "x"), file("p.lst", "y")};
    q.way = Way::Dialog;
    q.upload = true;
    const CallId c = r.open(q);
    r.fixture.callWhileBusy();
    CHECK(r.listing.opened == std::vector<std::string>{"q.lst"});   // the first as handed over
    CHECK(r.fixture.fe.reply(c) && r.fixture.fe.reply(c)->done);
    // Without an upload folder: only the first, alone (as its content).
    OpenRig plain("test-dot");
    const CallId d = plain.open(OpenRequest{{file("a.test-seg", header("test-seg")), file("b.lst", "x")}, Way::Dialog, true, false});
    plain.fixture.callWhileBusy();
    CHECK(plain.model() == "test-seg" && plain.listing.opened.empty());
    CHECK(plain.fixture.fe.reply(d) && plain.fixture.fe.reply(d)->done);
}

TEST(preset_alone_as_content_refused_when_it_names_files) {
    OpenRig r;
    const auto* a = r.opened(file("game.test-seg", keys("test-dot", "  - test-disk: { file: data.testdisk.yaml }\n")), Way::Drop, true);
    CHECK(a && !a->done && a->reason.find("handed over without its folder") != std::string::npos);
    CHECK(hasText(r.notices(), "handed over without its folder"));
}

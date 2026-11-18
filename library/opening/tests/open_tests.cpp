// The open part: the request, the reply, a medium, the file kept for later,
// several files; with the stand-ins of TestOpeners.hpp. Opening through the
// real presets and media parts is in real_parts_tests.cpp.
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "TestOpeners.hpp"
#include "sessions/LibraryTopics.hpp"
#include <algorithm>

using namespace calcu::opening;
using namespace calcu::opening::test;

namespace {
std::vector<std::string> logLines(OpenRig& r) {
    std::vector<std::string> out;
    for (const auto& p : r.rig.fe.mirror.takeAppended(calcu::sessions::kLogTopic))
        if (const auto* l = calcu::sessions::payloadAs<calcu::sessions::LogLine>(p)) out.push_back(l->text);
    return out;
}
bool isAs(const OpenReply* o, OpenedAs as, const std::string& detail = "") {
    return o && o->as == as && (detail.empty() || o->detail == detail);
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
    for (Way w : {Way::Drop, Way::OpenWith, Way::Start, Way::Dialog, Way::Harness}) CHECK(wayNamed(wayName(w)) == w);
    CHECK(!OpenRequest::fromValue(Value::object({{"way", "drop"}}), &why) && !why.empty());   // no files
    CHECK(!OpenRequest::fromValue(Value::object({{"files", Value(Value::Array{Value::object({{"name", "a"}})})}, {"way", "fly"}}), &why));
    const OpenReply o(OpenedAs::Medium, "w.tdisk.yaml", "drive");
    CHECK(o.toValue() == Value::object({{"as", "medium"}, {"file", "w.tdisk.yaml"}, {"detail", "drive"}}));
}

// ── The reply ──

TEST(open_reply_says_preset_medium_kind) {
    calcu::testing::TempFolder dir("calcu-open");
    dir.write("game.tseg", "model: test-dot\n");
    OpenRig r;
    r.rig.build(mt::kDot);
    const auto* p = r.opened(file("game.tseg", "model: test-dot\n", dir / "game.tseg"));
    CHECK(p && p->done && isAs(r.replyOf(p), OpenedAs::Preset));
    CHECK(r.replyOf(p) && r.replyOf(p)->file == "game.tseg");
    CHECK(r.presets.files.size() == 1 && r.presets.files[0] == dir / "game.tseg");   // from its folder
    const auto* up = r.opened(file("game.tseg", "model: test-dot\n", "/uploads/game.tseg"), Way::Drop, true);
    CHECK(up && up->done && r.presets.contents.size() == 1);   // an upload: as its content
    const auto* nopath = r.opened(file("other.tseg", "model: test-dot\n"));
    CHECK(nopath && nopath->done && r.presets.contents.size() == 2);   // no path: as its content
    const auto* m = r.opened(file("w.tdisk.yaml", "content", dir / "w.tdisk.yaml"));
    CHECK(m && m->done && isAs(r.replyOf(m), OpenedAs::Medium, "drive"));
    const auto* k = r.opened(file("p.lst", "10 PRINT"));
    CHECK(k && k->done && isAs(r.replyOf(k), OpenedAs::FileKind, "test.listing"));
    const auto* b = r.opened(file("x.bin", "BLOB"));   // a kind that starts an operation: replied at its end
    CHECK(b && b->done && isAs(r.replyOf(b), OpenedAs::FileKind, "test.blob"));
}

TEST(open_reply_says_kept) {
    OpenRig r;
    r.rig.build();
    r.rig.part.operation = runningFor(200'000);
    r.rig.send("tp.op");
    const CallId c = r.open(file("p.lst", "x"));
    const auto* k = r.rig.fe.reply(c);   // at once
    CHECK(k && k->done && isAs(r.replyOf(k), OpenedAs::Kept));
    CHECK(r.replyOf(k) && r.replyOf(k)->toValue().find("as")->string() && *r.replyOf(k)->toValue().find("as")->string() == "kept");
}

TEST(open_reply_says_why_refused) {
    OpenRig r;
    r.rig.build();
    const auto* a = r.opened(file("photo.jpg", "x"));
    CHECK(a && !a->done && a->reason == "Test-Product cannot open photo.jpg." && !a->result);
    const auto* b = r.opened(file("x.bin", "BLOB refuse"));   // a kind's own refusal, with the notice added
    CHECK(b && !b->done && b->reason == "x.bin is a blob this product does not take.");
    CHECK(hasText(r.notices(), "x.bin is a blob this product does not take."));
}

// ── A medium ──

TEST(open_medium_checked_first_hint_passed_on) {
    OpenRig r;
    r.rig.build(mt::kDot);
    const auto* a = r.opened(file("old.tdisk.yaml", "outdated", "/m/old.tdisk.yaml"));
    CHECK(a && !a->done && a->reason.find("is outdated: set format-version: 2") != std::string::npos);
    CHECK(hasText(r.notices(), "old.tdisk.yaml is outdated"));
    CHECK(r.media.uses.empty() && r.media.imports == 0);
    const auto* up = r.opened(file("old.tdisk.yaml", "outdated"), Way::Drop, true);   // an upload: not imported either
    CHECK(up && !up->done && r.media.imports == 0);
}

TEST(open_medium_used_where_it_lies) {
    OpenRig r;
    r.rig.build(mt::kDot);
    const auto* a = r.opened(file("w.tdisk.yaml", "content", "/m/w.tdisk.yaml"));
    CHECK(a && a->done);
    CHECK(r.media.uses.size() == 1 && r.media.uses[0].how == "in-place" && r.media.uses[0].place == "drive");
    CHECK(r.media.uses.size() == 1 && r.media.uses[0].writtenBack);   // the file is the instance
    CHECK(r.media.imports == 0);   // nothing copied
}

TEST(open_template_not_written) {
    OpenRig r;
    r.rig.build(mt::kDot);
    CHECK(r.opened(file("blank.tdisk.yaml", "template: true", "/m/blank.tdisk.yaml"))->done);
    CHECK(r.media.uses.size() == 1 && !r.media.uses[0].writtenBack);
}

TEST(open_holding_place_inserted_after_eject) {
    OpenRig r;
    r.rig.build(mt::kDot);
    CHECK(r.opened(file("one.tdisk.yaml", "1", "/m/one.tdisk.yaml"))->done);
    CHECK(r.opened(file("two.tdisk.yaml", "2", "/m/two.tdisk.yaml"))->done);
    CHECK(r.media.ejected == std::vector<std::string>{"one.tdisk.yaml"});
    CHECK(r.media.held["drive"] == "two.tdisk.yaml");
}

TEST(open_module_one_slot_plugged) {
    OpenRig r;
    r.rig.build(mt::kSeg);
    const CallId c = r.open(file("m.tmod.yaml", "x", "/m/m.tmod.yaml"));
    CHECK(r.rig.session->busy());   // the attach of the slot's picker is an operation
    r.rig.callWhileBusy();
    const auto* a = r.rig.fe.reply(c);
    CHECK(a && a->done && isAs(r.replyOf(a), OpenedAs::Medium, "slot-1"));
}

TEST(open_module_two_slots_refused) {
    OpenRig r;
    r.rig.build(mt::kDot);
    const auto* a = r.opened(file("m.tmod.yaml", "x", "/m/m.tmod.yaml"));
    CHECK(a && !a->done && a->reason == "m.tmod.yaml fits more than one place; choose it in the picker of the place it should go into.");
    CHECK(hasText(r.notices(), "fits more than one place"));
    CHECK(r.media.uses.empty());
}

TEST(open_no_place_now_refused_says_where) {
    OpenRig r;
    r.rig.build(mt::kDot);
    const auto* a = r.opened(file("c.tcard.yaml", "x", "/m/c.tcard.yaml"));
    CHECK(a && !a->done);
    CHECK(a && a->reason == "The test-dot has no place for c.tcard.yaml now; a test card goes into the reader of the test-seg.");
    CHECK(r.media.uses.empty() && r.media.imports == 0);
}

TEST(open_upload_imported_then_placed) {
    OpenRig r;
    r.rig.build(mt::kDot);
    const auto* a = r.opened(file("w.tdisk.yaml", "x", "/uploads/w.tdisk.yaml"), Way::Drop, true);
    CHECK(a && a->done && isAs(r.replyOf(a), OpenedAs::Medium, "drive"));
    CHECK(r.media.imports == 1 && r.media.uses.size() == 1 && r.media.uses[0].how == "imported" && r.media.uses[0].place == "drive");
}

TEST(open_upload_name_taken_asks_then_replace) {
    OpenRig r;
    r.rig.build(mt::kDot);
    r.media.saved.insert("w.tdisk.yaml");
    const auto* a = r.opened(file("w.tdisk.yaml", "x"), Way::Drop, true);
    CHECK(a && !a->done && a->reason.rfind("asks: ", 0) == 0);
    CHECK(r.notices().empty());   // a question, not a notice
    CHECK(r.media.imports == 0 && r.media.uses.empty());
    const auto* yes = r.opened(file("w.tdisk.yaml", "x"), Way::Drop, true, true);
    CHECK(yes && yes->done && r.media.imports == 1 && r.media.held["drive"] == "w.tdisk.yaml");
}

TEST(no_drive_refused_in_place_imported_as_upload) {
    OpenRig r;
    r.rig.build(mt::kDot);
    r.media.noDrive = true;
    const auto* a = r.opened(file("w.tdisk.yaml", "x", "/m/w.tdisk.yaml"));
    CHECK(a && !a->done && a->reason.find("has no place for w.tdisk.yaml now") != std::string::npos);
    CHECK(r.media.imports == 0);   // not copied
    const auto* up = r.opened(file("w.tdisk.yaml", "x"), Way::Drop, true);
    CHECK(up && up->done && isAs(r.replyOf(up), OpenedAs::Medium, "imported"));
    CHECK(r.media.imports == 1 && r.media.uses.size() == 1 && r.media.uses[0].place.empty());
    CHECK(hasText(r.notices(), "has no place for w.tdisk.yaml now"));   // in the save folder, and told so
}

TEST(place_passing_medium_refuses_nothing_imported) {
    OpenRig r;
    r.rig.build(mt::kSeg);
    r.media.passing.insert("reader");
    const auto* a = r.opened(file("c.tcard.yaml", "x"), Way::Drop, true);
    CHECK(a && !a->done && a->reason == "A medium is passing through reader now.");
    CHECK(hasText(r.notices(), "passing through reader"));
    CHECK(r.media.imports == 0 && r.media.uses.empty());
}

TEST(no_answer_nothing_imported_nothing_placed) {
    OpenRig r;
    r.rig.build(mt::kDot);
    r.media.saved.insert("w.tdisk.yaml");
    const auto* a = r.opened(file("w.tdisk.yaml", "x"), Way::Drop, true);
    CHECK(a && a->reason.rfind("asks: ", 0) == 0);
    // The user says no: the frontend sends nothing more.
    r.rig.call();
    CHECK(r.media.imports == 0 && r.media.uses.empty() && r.media.held.empty());
}

// ── Kept for later ──

TEST(kept_while_operation_runs_opened_when_free) {
    OpenRig r;
    r.rig.build();
    r.rig.part.operation = runningFor(200'000);
    const CallId op = r.rig.send("tp.op");
    const CallId c = r.open(file("x.bin", "BLOB"));
    CHECK(isAs(r.replyOf(r.rig.fe.reply(c)), OpenedAs::Kept));
    CHECK(r.blob.opened.empty());
    r.rig.callWhileBusy();   // the operation ends, then the kept file starts its own
    CHECK(r.rig.fe.reply(op) && r.rig.fe.reply(op)->done);
    if (r.blob.opened.empty()) r.rig.call();
    r.rig.callWhileBusy();
    CHECK(r.blob.opened == std::vector<std::string>{"x.bin"});
    CHECK_EQ(r.rig.fe.replies(c), 1);   // replied once, with Kept
    CHECK(!r.rig.session->busy());
}

TEST(kept_before_machine_built) {
    OpenRig r;   // no machine yet: the start has not finished
    const CallId c = r.open(file("p.lst", "x"));
    CHECK(isAs(r.replyOf(r.rig.fe.reply(c)), OpenedAs::Kept));
    r.rig.call();
    CHECK(r.listing.opened.empty());
    r.rig.build();
    r.rig.call();
    CHECK(r.listing.opened == std::vector<std::string>{"p.lst"});
    r.rig.call();
    CHECK(r.listing.opened.size() == 1);   // once
}

TEST(second_kept_file_replaces_first) {
    OpenRig r;
    r.rig.build();
    r.rig.part.operation = runningFor(200'000);
    r.rig.send("tp.op");
    logLines(r);
    r.open(file("first.lst", "x"));
    const CallId second = r.open(file("second.lst", "x"));
    CHECK(isAs(r.replyOf(r.rig.fe.reply(second)), OpenedAs::Kept));
    CHECK(hasText(logLines(r), "first.lst"));   // a log line, no notice
    CHECK(r.notices().empty());
    r.rig.callWhileBusy();
    r.rig.call();
    CHECK(r.listing.opened == std::vector<std::string>{"second.lst"});
}

TEST(kept_file_refused_later_is_a_notice) {
    OpenRig r;
    r.rig.build();
    r.rig.part.operation = runningFor(200'000);
    r.rig.send("tp.op");
    const CallId c = r.open(file("photo.jpg", "x"));
    r.rig.callWhileBusy();
    r.rig.call();
    CHECK(hasText(r.notices(), "Test-Product cannot open photo.jpg."));
    CHECK_EQ(r.rig.fe.replies(c), 1);   // no second reply
}

// ── Several files ──

TEST(several_files_one_folder_preset_found_by_path) {
    calcu::testing::TempFolder up("calcu-uploads");
    OpenRig r([&](OpenSetup& s) { s.uploadFolder = up.str(); });
    r.rig.build(mt::kDot);
    OpenRequest q;
    q.files = {file("data.tdisk.yaml", "disk"), file("game.tseg", "model: test-dot\nfile: data.tdisk.yaml\n")};
    q.way = Way::Dialog;
    q.upload = true;
    const CallId c = r.open(q);
    r.rig.callWhileBusy();
    CHECK(r.rig.fe.reply(c) && r.rig.fe.reply(c)->done && isAs(r.replyOf(r.rig.fe.reply(c)), OpenedAs::Preset));
    CHECK(r.presets.files.size() == 1 && r.presets.contents.empty());
    if (r.presets.files.size() != 1) return;
    const std::filesystem::path p(r.presets.files[0]);
    CHECK(p.filename() == "game.tseg");
    CHECK(std::filesystem::exists(p.parent_path() / "data.tdisk.yaml"));   // the files it names are beside it
    CHECK(p.parent_path().parent_path() == up.path());
    CHECK(p.parent_path().filename().string().rfind("open-", 0) == 0);
}

TEST(several_files_first_preset_by_name) {
    calcu::testing::TempFolder up("calcu-uploads");
    OpenRig r([&](OpenSetup& s) { s.uploadFolder = up.str(); });
    r.rig.build(mt::kDot);
    OpenRequest q;
    q.files = {file("b.tseg", "model: test-seg\n"), file("notes.txt", "model: test-dot\n"), file("a.tdot", "model: test-dot\n")};
    q.way = Way::Dialog;
    q.upload = true;
    r.open(q);
    r.rig.callWhileBusy();
    CHECK(r.presets.files.size() == 1 && std::filesystem::path(r.presets.files[0]).filename() == "a.tdot");
    // None with a preset extension: the first by name of those with a model line.
    q.files = {file("z.txt", "model: test-dot\n"), file("m.txt", "model: test-seg\n"), file("x.lst", "x")};
    r.open(q);
    r.rig.callWhileBusy();
    CHECK(r.presets.files.size() == 2 && std::filesystem::path(r.presets.files[1]).filename() == "m.txt");
}

TEST(several_files_no_preset_first_file_alone) {
    calcu::testing::TempFolder up("calcu-uploads");
    OpenRig r([&](OpenSetup& s) { s.uploadFolder = up.str(); });
    r.rig.build();
    OpenRequest q;
    q.files = {file("q.lst", "x"), file("p.lst", "y")};
    q.way = Way::Dialog;
    q.upload = true;
    const CallId c = r.open(q);
    r.rig.callWhileBusy();
    CHECK(r.listing.opened == std::vector<std::string>{"q.lst"});   // the first as handed over
    CHECK(r.rig.fe.reply(c) && r.rig.fe.reply(c)->done);
    // Without an upload folder: only the first, alone.
    OpenRig plain;
    plain.rig.build();
    const CallId d = plain.open(OpenRequest{{file("a.tseg", "model: test-seg\n"), file("b.lst", "x")}, Way::Dialog, true, false});
    plain.rig.callWhileBusy();
    CHECK(plain.presets.contents == std::vector<std::string>{"a.tseg"} && plain.listing.opened.empty());
    CHECK(plain.rig.fe.reply(d) && plain.rig.fe.reply(d)->done);
}

TEST(preset_alone_as_content_refused_when_it_names_files) {
    OpenRig r;
    r.rig.build();
    const auto* a = r.opened(file("game.tseg", "model: test-seg\nfile: data.tdisk.yaml\n"), Way::Drop, true);
    CHECK(a && !a->done && a->reason.find("names other files") != std::string::npos);
    CHECK(hasText(r.notices(), "game.tseg names other files"));
}

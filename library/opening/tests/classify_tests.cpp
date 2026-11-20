// Classifying a file: the order of its tests (Classify.hpp), the preset by
// its content, the limits per way in; an outdated preset, an unreadable file.
#include "OpenRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::opening;
using namespace calcu::opening::test;
using What = Classified::What;

namespace {
// The rig's setup, to change for one case.
struct Setup {
    OpenRig r;
    OpenSetup s = r.part->setup();
    Classified operator()(const OpenedFile& f, Way w = Way::Drop) const { return classify(f, w, s); }
};
} // namespace

TEST(unreadable_large_hidden_refused_first) {
    Setup c;
    c.s.maxBytes[Way::Drop] = 8;
    // 1a: the host could not read it, whatever its name.
    const Classified a = c(OpenedFile{"game.test-seg", "/x/game.test-seg", std::nullopt});
    CHECK(a.what == What::Refused);
    CHECK_EQ(a.refusal, std::string("game.test-seg could not be read."));
    // 1b: larger than the limit, before its name is looked at.
    const Classified b = c(file("game.test-seg", "model: test-seg\n"));
    CHECK(b.what == What::Refused);
    CHECK_EQ(b.refusal, std::string("game.test-seg is larger than 8 bytes and is not opened."));
    // 1c: a hidden file, even with a preset extension.
    const Classified h = c(file(".game.test-seg", "x"));
    CHECK(h.what == What::Refused);
    CHECK_EQ(h.refusal, std::string(".game.test-seg is a hidden file and is not opened."));
}

TEST(preset_extension_before_medium_and_kinds) {
    Setup c;
    // A name that would be a medium and a kind as well: the preset extension is asked first.
    CHECK(c(file("x.testdisk.yaml.test-seg", "BLOB")).what == What::Preset);
    CHECK(c(file("game.test-dot", "nothing like a preset")).what == What::Preset);
}

TEST(medium_suffix_before_kinds) {
    Setup c;
    CHECK(c(file("work.testdisk.yaml", "BLOB model: x\n")).what == What::Medium);   // content of a blob: still a medium
    CHECK(c(file("card.testcard.yaml", "model: test-seg\n")).what == What::Medium);  // a model line: still a medium
}

TEST(file_kinds_in_product_order) {
    Setup c;
    const Classified both = c(file("both.lst", "BLOB"));   // both kinds take it: the first in the product's order
    CHECK(both.what == What::Kind && both.kind == &c.r.listing);
    const Classified blob = c(file("data.bin", "BLOB 123"));
    CHECK(blob.what == What::Kind && blob.kind == &c.r.blob);
    c.s.kinds = {&c.r.blob, &c.r.listing};
    const Classified swapped = c(file("both.lst", "BLOB"));
    CHECK(swapped.what == What::Kind && swapped.kind == &c.r.blob);
}

TEST(model_line_after_file_kinds) {
    Setup c;
    CHECK(c(file("notes.txt", "# a preset\nmodel: test-seg\n")).what == What::Preset);   // test 5
    const Classified k = c(file("prog.lst", "model: test-seg\n"));                        // a kind first
    CHECK(k.what == What::Kind && k.kind == &c.r.listing);
}

TEST(unknown_file_refused_with_notice_naming_it) {
    Setup c;
    const Classified n = c(file("photo.jpg", "\xff\xd8\xff"));
    CHECK(n.what == What::Refused);
    CHECK_EQ(n.refusal, std::string("Test-Product cannot open photo.jpg."));
    OpenRig r;
    const auto* reply = r.opened(file("photo.jpg", "\xff\xd8\xff"));
    CHECK(reply && !reply->done && reply->reason == "Test-Product cannot open photo.jpg.");
    const auto notices = r.notices();
    CHECK(notices.size() == 1 && notices[0] == "Test-Product cannot open photo.jpg.");
}

TEST(open_names_compared_without_case) {
    Setup c;
    CHECK(c(file("GAME.TEST-SEG", "x")).what == What::Preset);
    CHECK(c(file("Work.TestDisk.YAML", "x")).what == What::Medium);
    const Classified k = c(file("PROG.LST", "x"));
    CHECK(k.what == What::Kind && k.kind == &c.r.listing);
}

TEST(outdated_preset_extension_is_preset_refused_with_hint) {
    Setup c;
    const std::string old = "format-version: 1\nmodel: test-seg\nBLOB\n";
    CHECK(c(file("old.test-seg", old)).what == What::Preset);   // not tried as anything else
    OpenRig r;
    const auto* reply = r.opened(file("old.test-seg", old));
    CHECK(reply && !reply->done && reply->reason.find("old.test-seg") != std::string::npos);
    const auto notices = r.notices();   // the presets part's sentence, as the one notice
    CHECK(reply && notices.size() == 1 && notices[0] == reply->reason);
    CHECK(r.listing.opened.empty() && r.blob.opened.empty());
}

TEST(unreadable_file_refused_notice_names_it) {
    OpenRig r;
    const auto* reply = r.opened(OpenedFile{"lost.lst", "/gone/lost.lst", std::nullopt});
    CHECK(reply && !reply->done && reply->reason == "lost.lst could not be read.");
    CHECK(hasText(r.notices(), "lost.lst could not be read."));
    CHECK(r.listing.opened.empty());
}

// ── A preset by its model line ──

TEST(preset_by_model_line_in_first_column) {
    CHECK(looksLikePreset(bytesOf("# Hanoi\nmodel: PC-1600\nkeys:\n  - key: mode\n")));
    CHECK(looksLikePreset(bytesOf("model: PC-1500:A04\n")));
    CHECK(looksLikePreset(bytesOf("# \xE2\x80\x93" "empty\xE2\x80\x93\r\nmodel: PC-1600\r\n")));   // '–', CRLF
    CHECK(!looksLikePreset(bytesOf("")));
    CHECK(!looksLikePreset(bytesOf("models: x\nmodel")));
}

TEST(indented_or_commented_model_not_a_preset) {
    CHECK(!looksLikePreset(bytesOf("keys:\n  model: PC-1600\n")));
    CHECK(!looksLikePreset(bytesOf("# model: PC-1600\nkeys:\n")));
}

TEST(zero_byte_or_bad_utf8_not_a_preset) {
    std::vector<std::uint8_t> nul = bytesOf("model: PC-1600\n");
    nul.push_back(0);
    CHECK(!looksLikePreset(nul));
    CHECK(!looksLikePreset(bytesOf("model: PC-1600\n\xff\xfe")));
    CHECK(!looksLikePreset(bytesOf("model: \xC3\n")));   // a cut sequence
}

TEST(preset_by_model_line_bom_allowed) {
    CHECK(looksLikePreset(bytesOf("\xEF\xBB\xBFmodel: PC-1500\n")));
}

// ── The limits per way in ──

TEST(open_limit_per_way_in) {
    Setup c;
    c.s.maxBytes[Way::Drop] = 1024 * 1024;
    c.s.maxBytes[Way::OpenWith] = 1024 * 1024;
    const std::string big(1024 * 1024 + 1, 'x');
    const Classified d = c(file("big.lst", big), Way::Drop);
    CHECK(d.what == What::Refused && d.refusal == "big.lst is larger than 1 MiB and is not opened.");
    CHECK(c(file("big.lst", big), Way::OpenWith).what == What::Refused);
    CHECK(c(file("big.lst", std::string(1024 * 1024, 'x')), Way::Drop).what == What::Kind);   // the limit itself is taken
    OpenRigOptions o;
    o.adjust = [](OpenSetup& s) { s.maxBytes[Way::Drop] = 64 * 1024; };
    OpenRig r(o);
    const auto* reply = r.opened(file("big.lst", std::string(64 * 1024 + 1, 'x')));
    CHECK(reply && !reply->done && reply->reason == "big.lst is larger than 64 KiB and is not opened.");
    CHECK(hasText(r.notices(), "larger than 64 KiB"));
}

TEST(way_without_limit_takes_any_size) {
    Setup c;
    c.s.maxBytes[Way::Drop] = 10;
    const std::string big(4096, 'x');
    CHECK(c(file("big.lst", big), Way::Dialog).what == What::Kind);
    CHECK(c(file("big.lst", big), Way::Harness).what == What::Kind);
    CHECK(c(file("big.lst", big), Way::Start).what == What::Kind);
    c.s.maxBytes[Way::Dialog] = 1500;
    CHECK_EQ(c(file("big.lst", big), Way::Dialog).refusal, std::string("big.lst is larger than 1500 bytes and is not opened."));
}

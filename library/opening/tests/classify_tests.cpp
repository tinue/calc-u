// LIB-13 L3 to L5 (design §6): the order of the tests of AC-1, the preset by
// its content (AC-2), the limits per way in (AC-10); EC-1, EC-5.
#include "TestHarness.hpp"
#include "TestOpeners.hpp"

using namespace calcu::opening;
using namespace calcu::opening::test;
using What = Classified::What;

namespace {
struct Setup {
    TestPresets presets;
    TestMedia media;
    ListingKind listing;
    BlobKind blob;
    OpenSetup s = standInSetup(presets, media, listing, blob);
    Classified operator()(const OpenedFile& f, Way w = Way::Drop) const { return classify(f, w, s); }
};
} // namespace

TEST(ac1_unreadable_large_hidden_refused_first) {
    Setup c;
    c.s.maxBytes[Way::Drop] = 8;
    // 1a: the host could not read it, whatever its name.
    const Classified a = c(OpenedFile{"game.tseg", "/x/game.tseg", std::nullopt});
    CHECK(a.what == What::Refused);
    CHECK_EQ(a.refusal, std::string("game.tseg could not be read."));
    // 1b: larger than the limit, before its name is looked at.
    const Classified b = c(file("game.tseg", "model: test-seg\n"));
    CHECK(b.what == What::Refused);
    CHECK_EQ(b.refusal, std::string("game.tseg is larger than 8 bytes and is not opened."));
    // 1c: a hidden file, even with a preset extension.
    const Classified h = c(file(".game.tseg", "x"));
    CHECK(h.what == What::Refused);
    CHECK_EQ(h.refusal, std::string(".game.tseg is a hidden file and is not opened."));
}

TEST(ac1_preset_extension_before_medium_and_kinds) {
    Setup c;
    // A name that would be a medium and a kind as well: the preset extension is asked first.
    CHECK(c(file("x.tdisk.yaml.tseg", "BLOB")).what == What::Preset);
    CHECK(c(file("game.tdot", "nothing like a preset")).what == What::Preset);
}

TEST(ac1_medium_suffix_before_kinds) {
    Setup c;
    CHECK(c(file("work.tdisk.yaml", "BLOB model: x\n")).what == What::Medium);   // content of a blob: still a medium
    CHECK(c(file("card.tcard.yaml", "model: test-seg\n")).what == What::Medium);  // a model line: still a medium
}

TEST(ac1_kinds_in_product_order) {
    Setup c;
    const Classified both = c(file("both.lst", "BLOB"));   // both kinds take it: the first in the product's order
    CHECK(both.what == What::Kind && both.kind == &c.listing);
    const Classified blob = c(file("data.bin", "BLOB 123"));
    CHECK(blob.what == What::Kind && blob.kind == &c.blob);
    c.s.kinds = {&c.blob, &c.listing};
    const Classified swapped = c(file("both.lst", "BLOB"));
    CHECK(swapped.what == What::Kind && swapped.kind == &c.blob);
}

TEST(ac1_model_line_after_kinds) {
    Setup c;
    CHECK(c(file("notes.txt", "# a preset\nmodel: test-seg\n")).what == What::Preset);   // test 5
    const Classified k = c(file("prog.lst", "model: test-seg\n"));                        // a kind first
    CHECK(k.what == What::Kind && k.kind == &c.listing);
}

TEST(ac1_nothing_refused_with_notice_naming_file) {
    Setup c;
    const Classified n = c(file("photo.jpg", "\xff\xd8\xff"));
    CHECK(n.what == What::Refused);
    CHECK_EQ(n.refusal, std::string("Test-Product cannot open photo.jpg."));
    OpenRig r;
    r.rig.build();
    const auto* reply = r.opened(file("photo.jpg", "\xff\xd8\xff"));
    CHECK(reply && !reply->done && reply->reason == "Test-Product cannot open photo.jpg.");
    const auto notices = r.notices();
    CHECK(notices.size() == 1 && notices[0] == "Test-Product cannot open photo.jpg.");
}

TEST(ac1_names_compared_without_case) {
    Setup c;
    CHECK(c(file("GAME.TSEG", "x")).what == What::Preset);
    CHECK(c(file("Work.TDisk.YAML", "x")).what == What::Medium);
    const Classified k = c(file("PROG.LST", "x"));
    CHECK(k.what == What::Kind && k.kind == &c.listing);
}

TEST(ec1_outdated_preset_extension_is_preset_refused_with_hint) {
    Setup c;
    CHECK(c(file("old.tseg", "format-version: 1\noutdated\n")).what == What::Preset);   // not tried as anything else
    OpenRig r;
    r.rig.build();
    const auto* reply = r.opened(file("old.tseg", "format-version: 1\noutdated\n"));
    CHECK(reply && !reply->done);
    CHECK(reply && reply->reason.find("old.tseg is a preset of format 1") == 0);
    CHECK(hasText(r.notices(), "set format-version: 2"));   // the hint, as a notice
    CHECK(r.listing.opened.empty() && r.blob.opened.empty() && r.media.reads == 0);
}

TEST(ec5_unreadable_file_refused_notice_names_it) {
    OpenRig r;
    r.rig.build();
    const auto* reply = r.opened(OpenedFile{"lost.lst", "/gone/lost.lst", std::nullopt});
    CHECK(reply && !reply->done && reply->reason == "lost.lst could not be read.");
    CHECK(hasText(r.notices(), "lost.lst could not be read."));
    CHECK(r.listing.opened.empty());
}

// ── AC-2 (the cases of Calc-U-1600 tests/program_file_tests.cpp:198-208) ──

TEST(ac2_model_line_in_first_column) {
    CHECK(looksLikePreset(bytesOf("# Hanoi\nmodel: PC-1600\nkeys:\n  - key: mode\n")));
    CHECK(looksLikePreset(bytesOf("model: PC-1500:A04\n")));
    CHECK(looksLikePreset(bytesOf("# \xE2\x80\x93" "empty\xE2\x80\x93\r\nmodel: PC-1600\r\n")));   // '–', CRLF
    CHECK(!looksLikePreset(bytesOf("")));
    CHECK(!looksLikePreset(bytesOf("models: x\nmodel")));
}

TEST(ac2_indented_or_commented_model_not_a_preset) {
    CHECK(!looksLikePreset(bytesOf("keys:\n  model: PC-1600\n")));
    CHECK(!looksLikePreset(bytesOf("# model: PC-1600\nkeys:\n")));
}

TEST(ac2_zero_byte_or_bad_utf8_not_a_preset) {
    std::vector<std::uint8_t> nul = bytesOf("model: PC-1600\n");
    nul.push_back(0);
    CHECK(!looksLikePreset(nul));
    CHECK(!looksLikePreset(bytesOf("model: PC-1600\n\xff\xfe")));
    CHECK(!looksLikePreset(bytesOf("model: \xC3\n")));   // a cut sequence
}

TEST(ac2_bom_allowed) {
    CHECK(looksLikePreset(bytesOf("\xEF\xBB\xBFmodel: PC-1500\n")));
}

// ── AC-10 ──

TEST(ac10_limit_per_way_in) {
    Setup c;
    c.s.maxBytes[Way::Drop] = 1024 * 1024;
    c.s.maxBytes[Way::OpenWith] = 1024 * 1024;
    const std::string big(1024 * 1024 + 1, 'x');
    const Classified d = c(file("big.lst", big), Way::Drop);
    CHECK(d.what == What::Refused && d.refusal == "big.lst is larger than 1 MiB and is not opened.");
    CHECK(c(file("big.lst", big), Way::OpenWith).what == What::Refused);
    CHECK(c(file("big.lst", std::string(1024 * 1024, 'x')), Way::Drop).what == What::Kind);   // the limit itself is taken
    OpenRig r([](OpenSetup& s) { s.maxBytes[Way::Drop] = 64 * 1024; });
    r.rig.build();
    const auto* reply = r.opened(file("big.lst", std::string(64 * 1024 + 1, 'x')));
    CHECK(reply && !reply->done && reply->reason == "big.lst is larger than 64 KiB and is not opened.");
    CHECK(hasText(r.notices(), "larger than 64 KiB"));
}

TEST(ac10_way_without_limit_takes_any_size) {
    Setup c;
    c.s.maxBytes[Way::Drop] = 10;
    const std::string big(4096, 'x');
    CHECK(c(file("big.lst", big), Way::Dialog).what == What::Kind);
    CHECK(c(file("big.lst", big), Way::Harness).what == What::Kind);
    CHECK(c(file("big.lst", big), Way::Start).what == What::Kind);
    CHECK_EQ(sizeText(1024 * 1024), std::string("1 MiB"));
    CHECK_EQ(sizeText(1500), std::string("1500 bytes"));
}

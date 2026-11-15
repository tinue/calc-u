// LIB-6 L2: header keys and places (AC-2 to AC-5, AC-8, AC-9).
#include "PresetRig.hpp"
#include "TestHarness.hpp"

using namespace calcu::presets;
using namespace calcu::presets::test;
namespace machine = calcu::machine;

namespace {
const LoadPlan* planOf(const std::variant<LoadPlan, ReadError>& r) { return std::get_if<LoadPlan>(&r); }
bool pluggedAt(mt::TestMachine* m, const char* place, const machine::PeripheralId& id) {
    if (!m) return false;
    for (const auto& e : m->plugged().byPlace)
        if (machine::placeText(e.place) == place && e.peripheral == id) return true;
    return false;
}
} // namespace

TEST(ac2_header_keys_and_defaults) {
    Reading rd;
    const auto a = rd.read(header(), "hanoi.test-seg");
    const LoadPlan* p = planOf(a);
    CHECK(p != nullptr);
    if (!p) return;
    CHECK_EQ(p->header.presetName, std::string("hanoi"));   // the file stem
    CHECK(!p->header.nameWritten);
    CHECK_EQ(p->header.description, std::string());
    CHECK_EQ(p->header.timeoutSeconds, std::int64_t(3600));
    CHECK(!p->variant);   // the model's default at the build
    const auto b = rd.read("description: Counts\ntimeout: 20\npreset-name: Counter\n" + header());   // anywhere
    const LoadPlan* q = planOf(b);
    CHECK(q && q->header.presetName == "Counter" && q->header.nameWritten);
    CHECK(q && q->header.description == "Counts" && q->header.timeoutSeconds == 20);
    CHECK(hasLine({rd.error(header() + "description: |\n  two\n  lines\n")}, "description: is one line."));
    CHECK(hasLine({rd.error(header() + "timeout: 0\n")}, "timeout: is a whole number of seconds, at least 1."));
    CHECK(hasLine({rd.error(header() + "timeout: 1.5\n")}, "timeout: is a whole number"));
}

TEST(ac2_preset_name_at_most_18_written_stem_any_length) {
    Reading rd;
    CHECK(planOf(rd.read(header() + "preset-name: abcdefghijklmnopqr\n")) != nullptr);   // 18
    CHECK(planOf(rd.read(header() + "preset-name: \xC3\xA9" "bcdefghijklmnopqr\n")) != nullptr);   // 18 code points
    CHECK_EQ(rd.error(header() + "preset-name: abcdefghijklmnopqrs\n"),
             std::string("t.test-seg, line 3: preset-name: has at most 18 characters."));
    const auto longStem = rd.read(header(), "a-file-name-of-thirty-chars.test-seg");
    CHECK(planOf(longStem) && planOf(longStem)->header.presetName == "a-file-name-of-thirty-chars");
}

TEST(ac2_place_keys_from_the_descriptor) {
    Reading rd;
    const auto seg = rd.read(header() + "slot-1: test-module\nprinter: test-printer\n");
    CHECK(planOf(seg) && planOf(seg)->places.size() == 2);
    const auto dot = rd.read(header("test-dot") + "a: test-hub\ncard-1: test-module\n");
    CHECK(planOf(dot) && planOf(dot)->places.size() == 2);
    // A place of the other model is no key here (AC-62).
    CHECK_EQ(rd.error(header() + "card-1: test-module\n"), std::string("t.test-seg, line 3: card-1 is not available on the test-seg."));
}

TEST(ac3_peripheral_fits_place_with_variant) {
    Reading rd;
    const auto r = rd.read(header() + "slot-1: test-module:8k\n");
    CHECK(planOf(r) && planOf(r)->places[0].request.variant == machine::VariantId{"8k"});
    CHECK_EQ(rd.error(header() + "slot-1: test-printer\n"),
             std::string("t.test-seg, line 3: The Test printer does not fit slot-1 of the test-seg."));
    CHECK_EQ(rd.error(header() + "slot-1: test-module:16k\n"),
             std::string("t.test-seg, line 3: The Test module has no variant 16k; its variants are 4k, 8k."));
}

TEST(ac3_module_by_name_reference) {
    Reading rd;
    const auto r = rd.read(header() + "slot-1: { name: TEST-MODULE }\n");
    CHECK(planOf(r) && planOf(r)->places[0].request.peripheral == mt::kModule);
    rd.rules.seg.places["slot-1"] = PlaceKeyRule{false, true, false};   // a product that takes name: only
    CHECK(hasLine({rd.error(header() + "slot-1: test-module\n")}, "slot-1: takes a reference"));
}

TEST(ac3_module_by_file_refused_without_a_resolver) {
    Reading rd;
    CHECK_EQ(rd.error(header() + "slot-1: { file: cards/m.card.yaml }\n"),
             std::string("t.test-seg, line 3: slot-1: a reference by file: is not read by this release; use name:."));
}

TEST(ac3_place_not_named_is_empty) {
    Rig r;
    CHECK(r.done(r.load(header() + "printer: test-printer\nslot-1: test-module\n")));
    CHECK(pluggedAt(r.machine(), "printer", mt::kPrinter));
    CHECK(r.done(r.load(header() + "slot-1: test-module\n")));
    CHECK(!pluggedAt(r.machine(), "printer", mt::kPrinter));   // ADR 0016: none
    CHECK(pluggedAt(r.machine(), "slot-1", mt::kModule));
    CHECK_EQ(r.machine()->plugged().byPlace.size(), std::size_t(1));
}

TEST(ac4_keys_repeats_in_file_order) {
    Rig r;
    const CallId c = r.load(header() + "keys:\n  - key: 2\n  - key: 3\nkeys:\n  - key: 2 3\n");
    CHECK(r.done(c));
    std::string order;
    for (const auto& e : r.machine()->keyEvents())
        if (e.down) order += e.key.value;
    CHECK_EQ(order, std::string("2323"));
}

TEST(ac5_model_from_model_key_not_extension) {
    Reading rd;
    const auto r = rd.read(header("test-seg"), "looks-like-dot.test-dot");
    CHECK(planOf(r) && planOf(r)->model == mt::kSeg);
    const auto t = rd.read(header("test-dot"), "any.txt");
    CHECK(planOf(t) && planOf(t)->model == mt::kDot);
}

TEST(ac8_missing_or_old_version_refused_as_outdated) {
    Reading rd;
    const std::string want =
        "t.test-seg: The file was written for an earlier release of Calc-U-Test and must be upgraded by hand to "
        "preset format 2, as docs/formats/preset.md describes it.";
    CHECK_EQ(rd.error("model: test-seg\nkeys:\n  - key: 1\n"), want);
    CHECK_EQ(rd.error("format-version: 1\nmodel: test-seg\n"), want);
    // Checked before any other key: an unknown key does not hide it.
    CHECK_EQ(rd.error("plotter: CE-1600P\nformat-version: 1\nmodel: test-seg\n"), want);
    CHECK_EQ(rd.error("MODEL: TI-59\nKEYSTROKES:\n"), want);   // a 2.3.0 state file
}

TEST(ac8_newer_version_refused) {
    Reading rd;
    CHECK_EQ(rd.error("format-version: 3\nmodel: test-seg\nnew-key: 1\n"),
             std::string("t.test-seg, line 1: The file was written by a newer release of Calc-U-Test (preset format "
                         "3); this release reads preset format 2."));
}

TEST(ac9_unknown_model_lists_the_models) {
    Reading rd;
    CHECK_EQ(rd.error("format-version: 2\nmodel: PC-1600\n"),
             std::string("t.test-seg, line 2: There is no model PC-1600 in Calc-U-Test; its models are test-seg and "
                         "test-dot."));
    CHECK_EQ(rd.error("format-version: 2\n"),
             std::string("t.test-seg: model: is required; the models are test-seg and test-dot."));
    CHECK_EQ(rd.error("format-version: 2\nmodel: test-seg:V2\n"), std::string("t.test-seg, line 2: The test-seg has no variants."));
}

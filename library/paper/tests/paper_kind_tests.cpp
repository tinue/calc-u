// LIB-8 T1, T2 (design §6): the test printers and the kind (AC-1).
#include "PaperTestKit.hpp"
#include "TestHarness.hpp"
#include "machine/Build.hpp"

using namespace calcu::paper;
using namespace calcu::paper::test;
namespace mt = calcu::machine::test;
namespace machine = calcu::machine;

namespace {
std::unique_ptr<mt::TestMachine> buildWith(const machine::ModelId& model, std::vector<machine::PeripheralRequest> ps) {
    mt::TestBuilder b;
    const machine::Catalogue& cat = mt::paperTestCatalogue();
    auto r = machine::build(cat, b, mt::requestFor(cat, model, std::nullopt, std::move(ps)));
    return std::unique_ptr<mt::TestMachine>(static_cast<mt::TestMachine*>(r.machine.release()));
}
machine::PeripheralRequest at(const char* place, const machine::PeripheralId& id) {
    return mt::peripheralAt(mt::paperTestCatalogue(), place, id);
}
} // namespace

TEST(test_printers_build_and_offer_paper_output) {
    auto m = buildWith(mt::kSeg, {at("printer", mt::kDotter), at("aux", mt::kPlotter)});
    CHECK(m != nullptr);
    if (!m) return;
    const auto caps = m->capabilities(PaperOutput::kKind);
    CHECK_EQ(caps.size(), std::size_t(2));
    CHECK(machine::capabilityAs<PaperOutput>(*m, machine::CapabilityId{"printer/paper-output"}) != nullptr);
    CHECK(machine::capabilityAs<PaperOutput>(*m, machine::CapabilityId{"aux/paper-output"}) != nullptr);
    // The shared test catalogue stays as it is: no test printer in it.
    CHECK(mt::testCatalogue().peripheral(mt::kDotter) == nullptr);
    CHECK_EQ(mt::paperTestCatalogue().peripheral(mt::kDotter)->name, std::string("TD-20"));
    CHECK_EQ(mt::paperTestCatalogue().peripheral(mt::kPlotter)->name, std::string("TP-4"));
    auto dot = buildWith(mt::kDot, {at("a", mt::kDotter)});   // fits test.chain too
    CHECK(dot && dot->capabilities(PaperOutput::kKind).size() == 1);
}

TEST(ac1_description_states_unit_width_form_drawing_resolution_text_layer) {
    auto m = buildWith(mt::kSeg, {at("printer", mt::kDotter), at("aux", mt::kPlotter)});
    if (!m) return;
    auto* td = machine::capabilityAs<PaperOutput>(*m, machine::CapabilityId{"printer/paper-output"});
    auto* tp = machine::capabilityAs<PaperOutput>(*m, machine::CapabilityId{"aux/paper-output"});
    CHECK(td && tp);
    if (!td || !tp) return;
    const PaperDescription& d = td->description();
    CHECK(d.unit == (Millimetres{1, 2}));                                      // unit in millimetres
    CHECK_EQ(d.width, units(40));                                               // width of the paper
    CHECK_EQ(d.printableLeft, units(2));                                        // the part printed on
    CHECK_EQ(d.printableWidth, units(36));
    CHECK(d.form == MarkForm::Dots);                                            // dots
    CHECK_EQ(d.dotDiameter, Coord(8'000));                                      // the size of a dot
    CHECK(d.darknessGreys == (std::vector<std::uint8_t>{160, 0}));              // the grey of each darkness
    CHECK_EQ(d.pictureDotsPerInch, 300);                                        // resolution of a copy
    CHECK(d.textLayer);                                                         // a text layer
    const PaperDescription& p = tp->description();
    CHECK(p.unit == (Millimetres{1, 10}));
    CHECK_EQ(p.width, units(600));
    CHECK(p.form == MarkForm::PenStrokes);                                      // pen strokes
    CHECK_EQ(p.strokeWidth, units(3));                                          // the width of a stroke
    CHECK_EQ(p.pictureDotsPerInch, 254);
    CHECK(!p.textLayer);
    // One paper output per printer, owned by it, named "<place>/paper-output".
    for (const auto& e : m->capabilities(PaperOutput::kKind)) {
        const auto* o = std::get_if<machine::PeripheralOwner>(&e.owner);
        CHECK(o != nullptr);
        if (o) CHECK_EQ(e.id.value, machine::placeText(o->place) + "/paper-output");
    }
    CHECK(!checkPaperOutput(*m, mt::kDotter, mt::td20Description()));
    CHECK(!checkPaperOutput(*m, mt::kPlotter, mt::tp4Description()));
}

TEST(ac1_conformance_check_finds_a_different_description) {
    auto m = buildWith(mt::kSeg, {at("printer", mt::kDotter)});
    if (!m) return;
    PaperDescription other = mt::td20Description();
    other.pictureDotsPerInch = 600;
    const auto problem = checkPaperOutput(*m, mt::kDotter, other);
    CHECK(problem.has_value());
    if (problem) CHECK_EQ(*problem, std::string("The paper description of test-dotter is not the one declared."));
    const auto none = checkPaperOutput(*m, mt::kPlotter, mt::tp4Description());
    CHECK(none.has_value());
    if (none) CHECK_EQ(*none, std::string("test-plotter offers no paper output."));
}

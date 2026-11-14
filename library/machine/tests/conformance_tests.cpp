#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include "machine/Conformance.hpp"

// checkAgainstDescriptor() (design §6, T13), used by the product tests of
// LIB-1 AC-33 and AC-40 from M-19 on.

using namespace calcu::machine;
using namespace calcu::machine::test;

TEST(conformance_passes_for_test_machines) {
    const Catalogue& cat = testCatalogue();
    auto seg = buildTestMachine(kSeg, std::nullopt,
                                {peripheralAt(cat, "slot-1", kModule), peripheralAt(cat, "printer", kPrinter)});
    auto v1 = buildTestMachine(kDot, VariantId{"V1"});
    auto v2 = buildTestMachine(kDot, VariantId{"V2"});
    CHECK(seg && v1 && v2);
    if (!seg || !v1 || !v2) return;
    for (TestMachine* m : {seg.get(), v1.get(), v2.get()}) {
        const auto problems = checkAgainstDescriptor(*m, cat);
        for (const auto& p : problems) std::fprintf(stderr, "  %s\n", p.c_str());
        CHECK(problems.empty());
        CHECK(m->keysDown().empty());   // every key it pressed it released again
    }
}

TEST(conformance_names_a_wrong_key_list) {
    auto m = buildTestMachine(kSeg);
    CHECK(m != nullptr);
    if (!m) return;
    Catalogue other = makeTestCatalogue();
    MachineDescriptor seg = *other.model(kSeg);
    seg.keys.push_back(KeyDescriptor{KeyId{"9"}, {}});
    Catalogue wrong({seg, *other.model(kDot)}, other.peripherals());
    const auto problems = checkAgainstDescriptor(*m, wrong);
    CHECK(!problems.empty());
    bool named = false;
    for (const auto& p : problems) if (p.find("\"9\"") != std::string::npos) named = true;
    CHECK(named);
}

#include "TestHarness.hpp"
#include "TestMachines.hpp"
#include <variant>

// LIB-1 AC-24 (design §6, T1). The build side is the CMake target
// calcu_library_tests, which links calcu_library and the test runner only,
// and the CI job `library` of both product workflows, which configures with
// CALCU_PRODUCT_59=OFF and CALCU_PRODUCT_1600=OFF. This case checks that the
// machines built for the tests have, between them, what AC-24 names.

using namespace calcu::machine;
using namespace calcu::machine::test;

TEST(ac24_library_tests_run_without_products) {
    const Catalogue& cat = testCatalogue();
    bool segment = false, dotMatrix = false, sound = false;
    for (const auto& m : cat.models()) {
        segment |= std::holds_alternative<SegmentGeometry>(m.display.geometry);
        dotMatrix |= std::holds_alternative<DotMatrixGeometry>(m.display.geometry);
        sound |= m.hasSound;
    }
    CHECK(segment && dotMatrix && sound);
    CHECK(!cat.peripherals().empty());
    // A product kind, listed by a running machine.
    auto m = buildTestMachine(kSeg, std::nullopt, {peripheralAt(cat, "printer", kPrinter)});
    CHECK(m && capabilitiesOf<TestCounter>(*m).size() == 1);
    CHECK(TestCounter::kKind.value.find('.') != std::string_view::npos);   // a product's prefix
}

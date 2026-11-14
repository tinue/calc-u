#include "TestHarness.hpp"
#include <cstring>

// The runner of every product's C++ test program (ADR 0038): runs every
// registered TEST. An optional argument filters by substring of the test
// name. CTest runs the programs from their product directory so tests can
// read roms/ and examples/.

int main(int argc, char** argv) {
    const char* filter = argc > 1 ? argv[1] : nullptr;
    int ran = 0;
    for (auto& c : testharness::registry()) {
        if (filter && !std::strstr(c.name, filter)) continue;
        int before = testharness::failCount();
        testharness::currentName() = c.name;
        c.fn();
        ++ran;
        if (testharness::failCount() != before) std::fprintf(stderr, "  in %s\n", c.name);
    }
    std::printf("%d tests (%d skipped), %d checks passed, %d failed\n", ran, testharness::skipCount(),
                testharness::passCount(), testharness::failCount());
    return testharness::failCount() == 0 ? 0 : 1;
}

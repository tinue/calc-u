#include "TestHarness.hpp"

// The runner of every product's C++ test program (TestHarness.hpp has its
// arguments). CTest runs the programs from their product directory so
// tests can read roms/ and examples/.

int main(int argc, char** argv) { return testharness::runTests(argc, argv); }

# The library (ADR 0002): code used by more than one product. Empty until
# the first shared feature moves in (docs/migration-plan.md, Part C1); the
# products link calcu_library so that they build against it from the start.
add_library(calcu_library INTERFACE)
target_compile_features(calcu_library INTERFACE cxx_std_17)

# The one C++ test runner (ADR 0038): testing/TestHarness.hpp (TEST, CHECK,
# CHECK_EQ, SKIP_UNLESS) and the main() that runs every registered TEST. A
# product's test program links calcu_test_runner.
if(CALCU_TESTS AND NOT EMSCRIPTEN)
  add_library(calcu_test_runner OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/testing/TestMain.cpp)
  target_include_directories(calcu_test_runner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/testing)
  target_compile_features(calcu_test_runner PUBLIC cxx_std_17)
endif()

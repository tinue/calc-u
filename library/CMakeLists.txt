# The library (ADR 0002): code used by more than one product. One static
# library, calcu_library, with one folder per area (library/<area>/,
# namespace calcu::<area>; LIB-1 design §2a). Includes are rooted at
# library/: #include "machine/Machine.hpp". The products link calcu_library.
file(GLOB CALCU_LIBRARY_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/machine/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/settings/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pacing/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sessions/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/*.cpp
)
# LIB-2: a host link of a serial port is a pseudo-terminal on macOS and Linux
# (PtyHostLink.cpp); elsewhere there is none (NoHostLink.cpp).
if((APPLE OR UNIX) AND NOT EMSCRIPTEN)
  set(CALCU_HAS_PTY ON)
  list(REMOVE_ITEM CALCU_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/NoHostLink.cpp)
else()
  set(CALCU_HAS_PTY OFF)
  list(REMOVE_ITEM CALCU_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/PtyHostLink.cpp)
endif()
add_library(calcu_library STATIC ${CALCU_LIBRARY_SOURCES})
if(CALCU_HAS_PTY)
  find_package(Threads REQUIRED)
  target_link_libraries(calcu_library PUBLIC Threads::Threads)
endif()
target_include_directories(calcu_library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(calcu_library PUBLIC cxx_std_20)  # ADR 0005
# Calc-U-59's static libraries end up in a shared object (ti59headless).
set_target_properties(calcu_library PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(calcu_library PRIVATE /W4 /utf-8)
else()
  target_compile_options(calcu_library PRIVATE -Wall -Wextra)
endif()
if(EMSCRIPTEN)
  # One build/web tree serves both Web engines with native wasm exceptions
  # (LIB-1 design §2a, D-6).
  target_compile_options(calcu_library PRIVATE -fwasm-exceptions)
endif()

# The one C++ test runner (ADR 0038): testing/TestHarness.hpp (TEST, CHECK,
# CHECK_EQ, SKIP_UNLESS) and the main() that runs every registered TEST. A
# test program links calcu_test_runner.
if(CALCU_TESTS AND NOT EMSCRIPTEN)
  add_library(calcu_test_runner OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/testing/TestMain.cpp)
  target_include_directories(calcu_test_runner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/testing)
  target_compile_features(calcu_test_runner PUBLIC cxx_std_20)

  # The library's own tests (Frequent; docs/test-groups.md): machines built
  # for the tests, never product code (LIB-1 AC-24). Sources:
  # library/<area>/tests/*.cpp.
  file(GLOB CALCU_LIBRARY_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/machine/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settings/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pacing/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sessions/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/tests/*.cpp
  )
  list(REMOVE_ITEM CALCU_LIBRARY_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/tests/pty_tests.cpp)
  add_executable(calcu_library_tests ${CALCU_LIBRARY_TEST_SOURCES})
  target_include_directories(calcu_library_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/machine/tests
                                                         ${CMAKE_CURRENT_SOURCE_DIR}/sessions/tests)
  target_link_libraries(calcu_library_tests PRIVATE calcu_library calcu_test_runner)
  if(MSVC)
    target_compile_options(calcu_library_tests PRIVATE /utf-8)
  else()
    target_compile_options(calcu_library_tests PRIVATE -Wall -Wextra)
  endif()
  add_test(NAME calcu_library_tests COMMAND calcu_library_tests)
  set_tests_properties(calcu_library_tests PROPERTIES LABELS frequent)

  # LIB-2 T8: the host links on real pseudo-terminals (Frequent; macOS and
  # Linux only), a program of their own so that they can move to Longer.
  if(CALCU_HAS_PTY)
    add_executable(calcu_library_pty_tests ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/tests/pty_tests.cpp
                                           ${CMAKE_CURRENT_SOURCE_DIR}/machine/tests/TestMachines.cpp)
    target_include_directories(calcu_library_pty_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/machine/tests
                                                               ${CMAKE_CURRENT_SOURCE_DIR}/sessions/tests)
    target_link_libraries(calcu_library_pty_tests PRIVATE calcu_library calcu_test_runner)
    target_compile_options(calcu_library_pty_tests PRIVATE -Wall -Wextra)
    add_test(NAME calcu_library_pty_tests COMMAND calcu_library_pty_tests)
    set_tests_properties(calcu_library_pty_tests PROPERTIES LABELS frequent)
  endif()

  # LIB-9 T-14: the self-test of the platform-store source check the
  # products run over their frontends (Frequent).
  if(NOT WIN32)
    add_test(NAME LibraryPlatformStoreCheck
             COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/settings/tests/check_platform_stores_test.sh)
    set_tests_properties(LibraryPlatformStoreCheck PROPERTIES LABELS frequent)
  endif()
endif()

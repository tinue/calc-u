# The library (ADR 0002): code used by more than one product. One static
# library, calcu_library, with one folder per area (library/<area>/,
# namespace calcu::<area>; LIB-1 design §2a). Includes are rooted at
# library/: #include "machine/Machine.hpp". The products link calcu_library.
# The C++ areas, once: their sources, their tests (<area>/tests/*.cpp) and the
# tests' include folders all come from this list. (web/ is the Web shell's,
# harness/cli/ and harness/attach/ are added below.)
set(CALCU_LIBRARY_AREAS machine files settings pacing sessions peripherals roms formats control startup typing presets paper debugger media opening parts harness shell)
set(CALCU_LIBRARY_SOURCES "")
foreach(area ${CALCU_LIBRARY_AREAS})
  file(GLOB area_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${area}/*.cpp)
  list(APPEND CALCU_LIBRARY_SOURCES ${area_sources})
endforeach()
# C59-5: harness attach (library/harness/attach/). attachPort() is always in
# (the client `calcu attach` uses it); the endpoint and the attached harness
# only with CALCU_ATTACH: POSIX, never Emscripten, never for an iOS device
# (C59-5 AC-13). On by default in a tree with the tests on macOS and Linux,
# so the attach tests run in Frequent and in CI; the Swift XCFramework sets
# it per slice (frontends/swift/build_xcframework.sh: Debug macOS and the
# simulator only).
list(APPEND CALCU_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/harness/attach/AttachPort.cpp
                                  ${CMAKE_CURRENT_SOURCE_DIR}/harness/attach/AttachClient.cpp)
if((APPLE OR UNIX) AND NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" AND CALCU_TESTS)
  set(CALCU_ATTACH_DEFAULT ON)
else()
  set(CALCU_ATTACH_DEFAULT OFF)
endif()
option(CALCU_ATTACH "Harness attach in calcu_library (C59-5; POSIX, development builds)" ${CALCU_ATTACH_DEFAULT})
if(CALCU_ATTACH)
  list(APPEND CALCU_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/harness/attach/LocalStream.cpp
                                    ${CMAKE_CURRENT_SOURCE_DIR}/harness/attach/AttachEndpoint.cpp
                                    ${CMAKE_CURRENT_SOURCE_DIR}/harness/attach/AttachedHarness.cpp)
endif()
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
# LIB-8 §3.3 (D-7): the paper picture gives the same pixels on every
# platform; no fused multiply-add (MSVC's /fp:precise, its default, keeps
# them apart already).
if(NOT MSVC)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/paper/Picture.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
if(CALCU_HAS_PTY OR CALCU_ATTACH)
  find_package(Threads REQUIRED)
  target_link_libraries(calcu_library PUBLIC Threads::Threads)
endif()
target_include_directories(calcu_library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(CALCU_ATTACH)
  target_compile_definitions(calcu_library PUBLIC CALCU_ATTACH=1)
endif()
target_compile_features(calcu_library PUBLIC cxx_std_20)  # ADR 0005
target_link_libraries(calcu_library PRIVATE calcu_warnings)
if(EMSCRIPTEN)
  # One build/web tree serves both Web engines with native wasm exceptions
  # (LIB-1 design §2a, D-6).
  target_compile_options(calcu_library PRIVATE -fwasm-exceptions)
endif()

# LIB-6: calcu_add_preset_check() for the suite checks of presets (AC-43, AC-51).
include(${CMAKE_CURRENT_SOURCE_DIR}/presets/PresetChecks.cmake)

# The one C++ test runner (ADR 0038): testing/TestHarness.hpp (TEST, CHECK,
# CHECK_EQ, SKIP_UNLESS) and the main() that runs every registered TEST. A
# test program links calcu_test_runner.
if(CALCU_TESTS AND NOT EMSCRIPTEN)
  add_library(calcu_test_runner OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/testing/TestMain.cpp)
  target_include_directories(calcu_test_runner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/testing)
  target_compile_features(calcu_test_runner PUBLIC cxx_std_20)

  # What the library's test programs share (calcu_library_tests,
  # calcu_library_pty_tests, the harness's calcu_test): the machines built for
  # the tests (machine/tests/TestMachines.cpp, compiled once) and the include
  # folders of the test stand-ins other areas' tests use (TestMachines.hpp,
  # TestParts.hpp, TestRomFiles.hpp, the harness's TestProduct.hpp; not
  # settings/tests, whose TestProduct.hpp is its own) and of library/testing/.
  add_library(calcu_test_machines OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/machine/tests/TestMachines.cpp
                                         ${CMAKE_CURRENT_SOURCE_DIR}/machine/tests/TestCpu.cpp
                                         ${CMAKE_CURRENT_SOURCE_DIR}/media/tests/TestKinds.cpp)
  target_link_libraries(calcu_test_machines PUBLIC calcu_library PRIVATE calcu_warnings)
  foreach(area machine sessions roms typing startup presets paper debugger media opening harness)
    target_include_directories(calcu_test_machines PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${area}/tests)
  endforeach()
  target_include_directories(calcu_test_machines PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/testing)

  # The library's own tests (Frequent; docs/test-groups.md): machines built
  # for the tests, never product code (LIB-1 AC-24). Sources:
  # library/<area>/tests/*.cpp.
  set(CALCU_LIBRARY_TEST_SOURCES "")
  foreach(area ${CALCU_LIBRARY_AREAS})
    file(GLOB area_tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${area}/tests/*.cpp)
    list(APPEND CALCU_LIBRARY_TEST_SOURCES ${area_tests})
  endforeach()
  list(REMOVE_ITEM CALCU_LIBRARY_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/machine/tests/TestMachines.cpp
                                              ${CMAKE_CURRENT_SOURCE_DIR}/machine/tests/TestCpu.cpp
                                              ${CMAKE_CURRENT_SOURCE_DIR}/media/tests/TestKinds.cpp)
  if(CALCU_ATTACH)
    file(GLOB CALCU_ATTACH_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/harness/attach/tests/*.cpp)
    list(APPEND CALCU_LIBRARY_TEST_SOURCES ${CALCU_ATTACH_TEST_SOURCES})
  endif()
  list(REMOVE_ITEM CALCU_LIBRARY_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/tests/pty_tests.cpp)
  add_executable(calcu_library_tests ${CALCU_LIBRARY_TEST_SOURCES})
  target_link_libraries(calcu_library_tests PRIVATE calcu_test_machines calcu_test_runner calcu_warnings)
  # LIB-6: the fixtures of the preset tests.
  target_compile_definitions(calcu_library_tests PRIVATE CALCU_PRESET_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/presets/tests/fixtures"
                                                     CALCU_PAPER_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/paper/tests/fixtures"
                                                     CALCU_SHELL_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/shell/tests")
  calcu_add_test(calcu_library_tests frequent COMMAND calcu_library_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # LIB-2 T8: the host links on real pseudo-terminals (Frequent; macOS and
  # Linux only), a program of their own so that they can move to Longer.
  if(CALCU_HAS_PTY)
    add_executable(calcu_library_pty_tests ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/tests/pty_tests.cpp)
    target_link_libraries(calcu_library_pty_tests PRIVATE calcu_test_machines calcu_test_runner calcu_warnings)
    calcu_add_test(calcu_library_pty_tests frequent COMMAND calcu_library_pty_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  # LIB-6 L14: the suite check of presets with calcu_test (Frequent): the
  # good fixtures and a template filled in by a fill script.
  calcu_add_preset_check(LibraryPresetFixtures PRODUCT test
                         FILES presets/tests/fixtures/good/* presets/tests/fixtures/fill/*.in
                         FILL presets/tests/fixtures/fill.cmake)

  # LIB-16 (design §6): the help tool's unittest cases (tools/help,
  # tools/helptool/tests/, on the fixture help trees), Frequent.
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_FOUND)
    calcu_add_test(LibraryHelpTool frequent
                   COMMAND ${Python3_EXECUTABLE} -I -B -m unittest discover -s helptool/tests -t .
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tools)
  endif()

  # LIB-9 T-14: the self-test of the platform-store source check the
  # products run over their frontends (Frequent).
  if(NOT WIN32)
    calcu_add_test(LibraryPlatformStoreCheck frequent
                   COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/settings/tests/check_platform_stores_test.sh
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    # LIB-14 L11: no product code in the shell and the component sets (AC-22).
    calcu_add_test(LibraryShellNoProduct frequent COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/qt6/tests/check_no_product_code.sh)
  endif()
endif()

# calcu_product_source_checks(PREFIX <CalcU59> ROM_ALLOW <path>…): the source
# checks every product runs over its own tree with the library's scripts
# (Frequent; docs/test-groups.md). <PREFIX>NoOwnRomLookup: no source looks for
# a ROM file outside the lookup but the files ROM_ALLOW names (LIB-12 AC-2;
# library/roms/tests/check_no_own_rom_lookup.sh, design T7).
# <PREFIX>NoPlatformSettings: no frontend keeps a setting in a platform store
# (UserDefaults, @AppStorage, QSettings, localStorage; LIB-9 AC-21, AC-31,
# AC-34; allow-list tools/platform_stores_allowed.txt). Needs bash: not on
# Windows, like the library's own check.
set(CALCU_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")
function(calcu_product_source_checks)
  cmake_parse_arguments(C "" "PREFIX" "ROM_ALLOW" ${ARGN})
  calcu_add_test(${C_PREFIX}NoOwnRomLookup frequent
                 COMMAND sh ${CALCU_LIBRARY_DIR}/roms/tests/check_no_own_rom_lookup.sh . ${C_ROM_ALLOW})
  if(NOT WIN32)
    calcu_add_test(${C_PREFIX}NoPlatformSettings frequent
                   COMMAND bash ${CALCU_LIBRARY_DIR}/settings/check_platform_stores.sh frontends
                           tools/platform_stores_allowed.txt)
  endif()
endfunction()

# LIB-15: the Web frontend shell (library/web/).
add_subdirectory(web)
# LIB-8 D-1, LIB-14 D-16: the Qt6 component set (library/qt6/, target
# calcu_qt6), only with the Qt6 frontends; its CMakeLists.txt finds Qt.
if(CALCU_FRONTEND_QT AND NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  add_subdirectory(qt6)
endif()
# LIB-11: the harness `calcu` and its test form `calcu_test`
# (library/harness/cli/), and calcu_harness_add_product() for the products.
add_subdirectory(harness/cli)

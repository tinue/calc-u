# The library: code used by more than one product. One static
# library, calcu_library, with one folder per area (library/<area>/,
# namespace calcu::<area>). Includes are rooted at
# library/: #include "machine/Machine.hpp". The products link calcu_library.
# The C++ areas, once: their sources, their tests (<area>/tests/*.cpp) and the
# tests' include folders all come from this list. (web/ is the Web shell's,
# harness/cli/ and harness/attach/ are added below.)
set(CALCU_LIBRARY_AREAS applog machine files settings pacing sessions peripherals roms formats control startup typing presets paper debugger media opening parts harness shell)
set(CALCU_LIBRARY_SOURCES "")
foreach(area ${CALCU_LIBRARY_AREAS})
  file(GLOB area_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${area}/*.cpp)
  list(APPEND CALCU_LIBRARY_SOURCES ${area_sources})
endforeach()
# Harness attach (library/harness/attach/). attachPort() is always in
# (the client `calcu attach` uses it); the endpoint and the attached harness
# only with CALCU_ATTACH: POSIX, never Emscripten, never for an iOS device.
# On by default in a tree with the tests on macOS and Linux,
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
option(CALCU_ATTACH "Harness attach in calcu_library (POSIX, development builds)" ${CALCU_ATTACH_DEFAULT})
if(CALCU_ATTACH)
  list(APPEND CALCU_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/harness/attach/LocalStream.cpp
                                    ${CMAKE_CURRENT_SOURCE_DIR}/harness/attach/AttachEndpoint.cpp
                                    ${CMAKE_CURRENT_SOURCE_DIR}/harness/attach/AttachedHarness.cpp)
endif()
# The Web engines' test-only commands: the
# Playwright tests' ways into a session that the page never sends
# (Calc-U-1600's `c1600.upload`, `c1600.display-text`). On by default, as
# for the test trees; Calc-U-1600's `frontends/web/build.sh --release` sets
# it OFF in build/web (only the files that hold them are compiled again),
# and its release packaging builds with `--release`. Calc-U-59 has none.
option(CALCU_WEB_TEST_AIDS "The Web engines' test-only commands (off in a release Web build)" ON)
# A host link of a serial port is a pseudo-terminal on macOS and Linux
# (PtyHostLink.cpp); elsewhere there is none (NoHostLink.cpp).
if((APPLE OR UNIX) AND NOT EMSCRIPTEN)
  set(CALCU_HAS_PTY ON)
  list(REMOVE_ITEM CALCU_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/NoHostLink.cpp)
else()
  set(CALCU_HAS_PTY OFF)
  list(REMOVE_ITEM CALCU_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/PtyHostLink.cpp)
endif()
# The native threaded host (shell/SessionThread.cpp) and the native app log
# (shell/HostAppLog.cpp): never in a Web build, whose session runs on its
# Worker and whose page and worker log on their own.
if(EMSCRIPTEN)
  list(REMOVE_ITEM CALCU_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shell/SessionThread.cpp
                                         ${CMAKE_CURRENT_SOURCE_DIR}/shell/HostAppLog.cpp)
endif()
add_library(calcu_library STATIC ${CALCU_LIBRARY_SOURCES})
# A development build starts the app log at info, a release build at warn
# (applog/AppLogLevel.cpp). Native: any build type but the release ones; the
# Web engine is always built as Release, so there it follows the test aids,
# which `--release` turns off.
if(EMSCRIPTEN)
  set(CALCU_DEV_BUILD_DEFAULT ${CALCU_WEB_TEST_AIDS})
elseif(CMAKE_BUILD_TYPE MATCHES "^(Release|MinSizeRel|RelWithDebInfo)$")
  set(CALCU_DEV_BUILD_DEFAULT OFF)
else()
  set(CALCU_DEV_BUILD_DEFAULT ON)
endif()
option(CALCU_DEV_BUILD "A development build: the app log starts at info" ${CALCU_DEV_BUILD_DEFAULT})
if(CALCU_DEV_BUILD)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/applog/AppLogLevel.cpp PROPERTIES COMPILE_DEFINITIONS CALCU_DEV_BUILD=1)
endif()
# The paper picture gives the same pixels on every
# platform; no fused multiply-add (MSVC's /fp:precise, its default, keeps
# them apart already).
if(NOT MSVC)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/paper/Picture.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
if(NOT EMSCRIPTEN)   # SessionThread, the pseudo-terminal link, harness attach
  find_package(Threads REQUIRED)
  target_link_libraries(calcu_library PUBLIC Threads::Threads)
endif()
target_include_directories(calcu_library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(CALCU_ATTACH)
  target_compile_definitions(calcu_library PUBLIC CALCU_ATTACH=1)
endif()
target_compile_features(calcu_library PUBLIC cxx_std_20)
target_link_libraries(calcu_library PRIVATE calcu_warnings)
if(EMSCRIPTEN)
  # One build/web tree serves both Web engines with native wasm exceptions.
  target_compile_options(calcu_library PRIVATE -fwasm-exceptions)
endif()

# calcu_add_preset_check() for the suite checks of presets.
include(${CMAKE_CURRENT_SOURCE_DIR}/presets/PresetChecks.cmake)

# The one C++ test runner: testing/TestHarness.hpp (TEST, CHECK,
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
  # for the tests, never product code. Sources:
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
  # The fixtures of the preset tests.
  target_compile_definitions(calcu_library_tests PRIVATE CALCU_PRESET_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/presets/tests/fixtures"
                                                     CALCU_PAPER_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/paper/tests/fixtures"
                                                     CALCU_SHELL_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/shell/tests")
  calcu_add_test(calcu_library_tests frequent COMMAND calcu_library_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  # The runner's Longer TESTs (TEST_IN(longer, …)): the harness giving up
  # on a key after an hour of emulated time.
  calcu_add_test(calcu_library_tests.longer longer COMMAND calcu_library_tests --group longer
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # The host links on real pseudo-terminals (Frequent; macOS and
  # Linux only), a program of their own so that they can move to Longer.
  if(CALCU_HAS_PTY)
    add_executable(calcu_library_pty_tests ${CMAKE_CURRENT_SOURCE_DIR}/peripherals/tests/pty_tests.cpp)
    target_link_libraries(calcu_library_pty_tests PRIVATE calcu_test_machines calcu_test_runner calcu_warnings)
    calcu_add_test(calcu_library_pty_tests frequent COMMAND calcu_library_pty_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  # The suite check of presets with calcu_test (Frequent): the
  # good fixtures and a template filled in by a fill script.
  calcu_add_preset_check(LibraryPresetFixtures PRODUCT test
                         FILES presets/tests/fixtures/good/* presets/tests/fixtures/fill/*.in
                         FILL presets/tests/fixtures/fill.cmake)

  # The help tool's unittest cases (tools/help,
  # tools/helptool/tests/, on the fixture help trees), Frequent.
  if(Python3_FOUND)
    calcu_add_test(LibraryHelpTool frequent
                   COMMAND ${Python3_EXECUTABLE} -I -B -m unittest discover -s helptool/tests -t .
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tools)
  endif()

  # The self-test of the platform-store source check the
  # products run over their frontends (Frequent).
  if(NOT WIN32)
    calcu_add_test(LibraryPlatformStoreCheck frequent
                   COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/settings/tests/check_platform_stores_test.sh
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    # No colour literal in the component sets (the products' chrome).
    calcu_add_test(LibraryChromeColours frequent COMMAND sh ${CMAKE_SOURCE_DIR}/tools/check_chrome_colours.sh .)
    # No product code in the shell and the component sets.
    calcu_add_test(LibraryShellNoProduct frequent COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/qt6/tests/check_no_product_code.sh)
  endif()
endif()

# calcu_product_source_checks(PREFIX <CalcU59> ROM_ALLOW <path>…): the source
# checks every product runs over its own tree with the library's scripts
# (Frequent; docs/test-groups.md). <PREFIX>NoOwnRomLookup: no source looks for
# a ROM file outside the lookup but the files ROM_ALLOW names
# (library/roms/tests/check_no_own_rom_lookup.sh). <PREFIX>NoOwnSession: the
# frontends host the library's app session, the product defines no session
# class of its own, the Qt6 frontend sends no suspend or continue
# (library/sessions/tests/check_no_own_session.sh).
# <PREFIX>NoPlatformSettings: no frontend keeps a setting in a platform store
# (UserDefaults, @AppStorage, QSettings, localStorage;
# allow-list tools/platform_stores_allowed.txt). Needs bash: not on
# Windows, like the library's own check.
set(CALCU_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")
function(calcu_product_source_checks)
  cmake_parse_arguments(C "" "PREFIX" "ROM_ALLOW" ${ARGN})
  calcu_add_test(${C_PREFIX}NoOwnRomLookup frequent
                 COMMAND sh ${CALCU_LIBRARY_DIR}/roms/tests/check_no_own_rom_lookup.sh . ${C_ROM_ALLOW})
  calcu_add_test(${C_PREFIX}NoOwnSession frequent
                 COMMAND sh ${CALCU_LIBRARY_DIR}/sessions/tests/check_no_own_session.sh .)
  if(NOT WIN32)
    calcu_add_test(${C_PREFIX}NoPlatformSettings frequent
                   COMMAND bash ${CALCU_LIBRARY_DIR}/settings/check_platform_stores.sh frontends
                           tools/platform_stores_allowed.txt)
  endif()
endfunction()

# The Web frontend shell (library/web/).
add_subdirectory(web)
# The Qt6 component set (library/qt6/, target
# calcu_qt6), only with the Qt6 frontends; its CMakeLists.txt finds Qt.
if(CALCU_FRONTEND_QT AND NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  add_subdirectory(qt6)
endif()
# The harness `calcu` and its test form `calcu_test`
# (library/harness/cli/), and calcu_harness_add_product() for the products.
add_subdirectory(harness/cli)

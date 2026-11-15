cmake_minimum_required(VERSION 3.21)

# Calc-U-59 in the repository's root CMake project (ADR 0003; option
# CALCU_PRODUCT_59): the emulation core, the app/ C++ layer the frontends
# use, the C++ test suite (CTest, with CALCU_TESTS), the headless harness
# library, and, with CALCU_FRONTEND_QT and when Qt6 is found, the Qt6
# desktop app via add_subdirectory(frontends/qt6). Under Emscripten only
# the core, the app layer and the Web engine (calcu59_web).
#
# The Swift app links CalcU59.xcframework, which frontends/swift/
# build_xcframework.sh builds from calcu59_core and calcu59_shared of this
# file (ADR 0013); Xcode compiles only Swift and the bridge. Every core/ or
# app/ change must pass the CMake, Xcode and Web builds — see CLAUDE.md
# § "Frontend Parity".

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "products/calc-u-59 is part of the root CMake project (ADR 0003): "
                      "configure the repository root, e.g. cmake -S ../.. -B ../../build/all")
endif()

project(Calc-U-59 LANGUAGES CXX)

# C++20 for the whole repository (ADR 0005).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CORE_ROOT   ${CMAKE_CURRENT_SOURCE_DIR}/core)
set(SHARED_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/app)

if(MSVC)
  # /utf-8: Shared/ has UTF-8 string literals (cue-card math tokens, mnemonics).
  set(CALCU59_WARNINGS /W4 /utf-8)
else()
  set(CALCU59_WARNINGS -Wall -Wextra)
endif()

# -----------------------------------------------------------------------------
# Core (hardware emulation) and Shared (frontend-neutral logic above it)
# -----------------------------------------------------------------------------
# The core and the TI machine behind the machine interface (C59-1): TIMachine,
# tiCatalogue(), the adapters of C59-1 design §5; the PC-100C and the Solid
# State ROM module as peripherals (C59-6).
add_library(calcu59_core STATIC
  ${CORE_ROOT}/PC100C.cpp
  ${CORE_ROOT}/SolidStateRomModule.cpp
  ${CORE_ROOT}/TI59Machine.cpp
  ${CORE_ROOT}/TMC0501.cpp
  ${CORE_ROOT}/ROM.cpp
  ${CORE_ROOT}/RAM.cpp
  ${CORE_ROOT}/TIAdapters.cpp
  ${CORE_ROOT}/TICatalogue.cpp
  ${CORE_ROOT}/TIKeys.cpp
  ${CORE_ROOT}/TIMachine.cpp
  ${CORE_ROOT}/TIRomText.cpp
  ${CORE_ROOT}/TIRomReader.cpp
  ${CORE_ROOT}/ModuleFile.cpp
)
target_include_directories(calcu59_core PUBLIC ${CORE_ROOT})
target_compile_options(calcu59_core PRIVATE ${CALCU59_WARNINGS})
set_target_properties(calcu59_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
# The core holds no lock and starts no thread (ADR 0009); the app layer's
# one thread is the Swift frontend's session thread (SessionThread, C59-5).
target_link_libraries(calcu59_core PUBLIC calcu_library)

file(GLOB SHARED_SOURCES CONFIGURE_DEPENDS ${SHARED_ROOT}/*.cpp)
list(REMOVE_ITEM SHARED_SOURCES ${SHARED_ROOT}/HarnessProduct.cpp)   # calcu59_harness below
if(EMSCRIPTEN)
  # The Swift frontend's session thread (C59-5): never in the Web engine.
  list(REMOVE_ITEM SHARED_SOURCES ${SHARED_ROOT}/SessionThread.cpp)
endif()
add_library(calcu59_shared STATIC ${SHARED_SOURCES})
target_include_directories(calcu59_shared PUBLIC ${SHARED_ROOT})
find_package(Threads REQUIRED)
target_link_libraries(calcu59_shared PUBLIC calcu59_core Threads::Threads)
target_compile_options(calcu59_shared PRIVATE ${CALCU59_WARNINGS})
# Both static libraries end up in the ti59headless shared object: on Linux
# that needs -fPIC (macOS always builds position-independent code).
set_target_properties(calcu59_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# Web engine (emcmake; frontends/web/build.sh copies it into
# frontends/web/dist/wasm/, reference/WebClient.md): core/ + app/ behind
# frontends/web/bindings.cpp. Single-threaded (no -pthread): a static host
# can't be relied on to send the headers SharedArrayBuffer needs, so
# SessionController runs in Threading::Cooperative. -fwasm-exceptions: app/
# reports ROM and file errors with exceptions; native wasm exceptions, as
# Calc-U-1600's engine and calcu_library use, so one build/web tree holds
# both engines (C59-1, since M-19; LIB-1 design §2a, D-6). -UNDEBUG: the
# core's asserts stay on, as in the em++ build this replaces (-O3 without
# -DNDEBUG).
# -----------------------------------------------------------------------------
if(EMSCRIPTEN)
  foreach(target calcu59_core calcu59_shared)
    target_compile_options(${target} PRIVATE -fwasm-exceptions -UNDEBUG)
  endforeach()
  add_executable(calcu59_web ${CMAKE_CURRENT_SOURCE_DIR}/frontends/web/bindings.cpp)
  target_link_libraries(calcu59_web PRIVATE calcu59_shared)
  target_compile_options(calcu59_web PRIVATE -fwasm-exceptions -UNDEBUG)
  set_target_properties(calcu59_web PROPERTIES OUTPUT_NAME ti59-core)
  target_link_options(calcu59_web PRIVATE -fwasm-exceptions -lembind -sMODULARIZE=1
                      -sEXPORT_NAME=createTI59CoreModule -sENVIRONMENT=web,worker
                      -sALLOW_MEMORY_GROWTH=1 -sNO_EXIT_RUNTIME=1
                      -sEXPORTED_RUNTIME_METHODS=FS)
  return()
endif()

# -----------------------------------------------------------------------------
# Headless harness (tools/headless/ti59.py loads it via ctypes; build.sh does
# the same with one clang++ call)
# -----------------------------------------------------------------------------
add_library(ti59headless SHARED ${CMAKE_CURRENT_SOURCE_DIR}/tools/headless/capi.cpp)
target_link_libraries(ti59headless PRIVATE calcu59_shared)

# The product entry of the harness `calcu` (LIB-11, M-28; library/harness/cli/):
# not in calcu59_shared, so the Swift and Web builds carry none of it. It
# finds the ROMs in the folder the tests use (LIB-11 design §8).
add_library(calcu59_harness STATIC ${SHARED_ROOT}/HarnessProduct.cpp)
target_link_libraries(calcu59_harness PUBLIC calcu59_shared)
target_compile_options(calcu59_harness PRIVATE ${CALCU59_WARNINGS})
# The state-file header's version is the apps' one: the Xcode project's
# MARKETING_VERSION, as the Qt and Web builds read it (user, 2026-11-15).
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/frontends/swift/Calc-U-59.xcodeproj/project.pbxproj
     CALCU59_VERSION_LINE REGEX "MARKETING_VERSION = [0-9.]+;" LIMIT_COUNT 1)
string(REGEX REPLACE ".*MARKETING_VERSION = ([0-9.]+).*" "\\1" CALCU59_VERSION "${CALCU59_VERSION_LINE}")
target_compile_definitions(calcu59_harness PRIVATE "CALCU59_ROM_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/roms\""
                                                   "CALCU59_VERSION=\"${CALCU59_VERSION}\"")
calcu_harness_add_product(calcu59_harness cu59::makeHarnessProduct)

# -----------------------------------------------------------------------------
# Tests (CALCU_TESTS): run from the product directory so they can read roms/
# and examples/
# -----------------------------------------------------------------------------
if(NOT MSVC)
  # The ROM-driven tests run millions of steps; keep Debug builds usable.
  target_compile_options(calcu59_core PRIVATE $<$<CONFIG:Debug>:-O1>)
endif()
if(CALCU_TESTS)
  file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
  add_executable(CalcU59Tests ${TEST_SOURCES})
  # The one test runner (ADR 0038): library/testing/, its main() included.
  target_link_libraries(CalcU59Tests PRIVATE calcu59_harness calcu_test_runner)
  target_compile_options(CalcU59Tests PRIVATE ${CALCU59_WARNINGS})
  if(NOT MSVC)
    target_compile_options(CalcU59Tests PRIVATE $<$<CONFIG:Debug>:-O1>)
  endif()

  enable_testing()
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)  # scratch files written by the tests
  # Test groups (ADR 0031, docs/test-groups.md): labels frequent, longer,
  # checkpoint; the last two only with CALCU_TESTS_LONGER / _CHECKPOINT.
  add_test(NAME CalcU59Tests COMMAND CalcU59Tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  set_tests_properties(CalcU59Tests PROPERTIES LABELS frequent)
  # tools/read_trace.py must read the trace CalcU59Tests wrote with Shared/TraceWriter.
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND)
    add_test(NAME CalcU59TraceReader
             COMMAND ${Python3_EXECUTABLE} tools/read_trace.py --json build/trace-test.bin
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(CalcU59TraceReader PROPERTIES DEPENDS CalcU59Tests LABELS frequent)
  endif()
  # C59-1 AC-17: the app layer reaches the machine through the interface only.
  add_test(NAME CalcU59CoreSeam COMMAND sh tests/check_core_seam.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  set_tests_properties(CalcU59CoreSeam PROPERTIES LABELS frequent)
  # LIB-3 AC-28, C59-5 AC-4 (design P59-1, B2): the Qt6 and Swift frontends use
  # none of the product's own session classes; no thread in app/ but the
  # session thread; the bridge only through it.
  add_test(NAME CalcU59NoOwnSession COMMAND sh tests/check_no_own_session.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  set_tests_properties(CalcU59NoOwnSession PROPERTIES LABELS frequent)
  # LIB-12 AC-2: no product source looks for a ROM file outside the lookup
  # (library/roms/tests/check_no_own_rom_lookup.sh; design T7). Allowed: the
  # product's lookUpRoms(), the catalogue's ROM entries, the Qt6 Bundle.
  add_test(NAME CalcU59NoOwnRomLookup
           COMMAND sh ${CMAKE_SOURCE_DIR}/library/roms/tests/check_no_own_rom_lookup.sh .
                   app/ProductRoms.cpp app/ProductRoms.hpp core/TICatalogue.cpp
                   frontends/qt6/app/QtResourceBundle.cpp
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  set_tests_properties(CalcU59NoOwnRomLookup PROPERTIES LABELS frequent)
  # No frontend keeps a setting in a platform store (UserDefaults,
  # @AppStorage, QSettings, localStorage; LIB-9 AC-21, AC-34, design T-24).
  # Needs bash: not on Windows, like the library's own check.
  if(NOT WIN32)
    add_test(NAME CalcU59NoPlatformSettings
             COMMAND bash ${CMAKE_SOURCE_DIR}/library/settings/check_platform_stores.sh frontends
                     tools/platform_stores_allowed.txt
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(CalcU59NoPlatformSettings PROPERTIES LABELS frequent)
  endif()
  if(Python3_FOUND)
    # tools/headless/ on the machine interface (C59-1 design T14): 5 + 7 = shows 12.
    add_test(NAME CalcU59HeadlessSmoke COMMAND ${Python3_EXECUTABLE} tools/headless/ti59.py 5 + 7 = --summary
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(CalcU59HeadlessSmoke PROPERTIES LABELS frequent
                         ENVIRONMENT TI59HEADLESS_LIB=$<TARGET_FILE:ti59headless>
                         PASS_REGULAR_EXPRESSION "display: \\[ *12\\.? *\\]" DEPENDS CalcU59Tests)
    # LIB-12 AC-7, AC-10 (design P5): ti59.py without one ROM file ends naming it.
    add_test(NAME CalcU59HeadlessRomMissing COMMAND ${Python3_EXECUTABLE} tests/headless_rom_tests.py
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(CalcU59HeadlessRomMissing PROPERTIES LABELS frequent
                         ENVIRONMENT TI59HEADLESS_LIB=$<TARGET_FILE:ti59headless> DEPENDS CalcU59Tests)
  endif()
  # The Xcode project compiles only Swift and the bridge (C59-5, ADR 0013).
  add_test(NAME CalcU59XcodeProjectCheck COMMAND frontends/swift/check_xcode_project.sh
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  set_tests_properties(CalcU59XcodeProjectCheck PROPERTIES LABELS frequent)

  # Longer: run locally before merging into main and at the end of a part.
  if(CALCU_TESTS_LONGER)
    # Against frontends/web/dist/ (frontends/web/build.sh first).
    add_test(NAME CalcU59WebPlaywright COMMAND frontends/web/run-tests.sh
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(CalcU59WebPlaywright PROPERTIES LABELS longer)
    # LIB-12, Technical Requirements (design P6): a build with one ROM file
    # taken away starts in neither headless nor the web client, naming it.
    if(NOT WIN32)
      add_test(NAME CalcU59RomMissingBuild COMMAND sh tests/rom_missing_build.sh
               WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      set_tests_properties(CalcU59RomMissingBuild PROPERTIES LABELS longer
                           ENVIRONMENT TI59HEADLESS_LIB=$<TARGET_FILE:ti59headless>)
    endif()
    if(APPLE)
      # XCUITest UIRegressionTests on one simulator (the iphone role of
      # bin/simulators.yaml).
      add_test(NAME CalcU59UIRegression
               COMMAND sh -c "xcodebuild test -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59 \
-testPlan UIRegressionTests -destination \"platform=iOS Simulator,id=$(bin/simulators resolve --role iphone)\""
               WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      set_tests_properties(CalcU59UIRegression PROPERTIES LABELS longer)
    endif()
  endif()

  # Checkpoint: run before a release.
  if(CALCU_TESTS_CHECKPOINT)
    add_test(NAME CalcU59PacingRealClock COMMAND CalcU59Tests pacing_longrun_real_clock
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(CalcU59PacingRealClock PROPERTIES LABELS checkpoint
                         ENVIRONMENT CALCU59_PACING_LONGRUN=600 TIMEOUT 900)
    if(APPLE)
      # The App Store set into appstore/screenshots (the iPhone Duo is opened
      # flat by hand) and the XCUITests on macOS.
      add_test(NAME CalcU59AppStoreScreenshots COMMAND bin/run-screenshots
               WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      add_test(NAME CalcU59UITestsMac
               COMMAND xcodebuild test -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59
                       -testPlan UIRegressionTests -destination platform=macOS
               WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      set_tests_properties(CalcU59AppStoreScreenshots CalcU59UITestsMac PROPERTIES LABELS checkpoint)
    endif()
  endif()
endif()

# -----------------------------------------------------------------------------
# Qt6 desktop app (CALCU_FRONTEND_QT; core/, app/ and the tests build without Qt)
# -----------------------------------------------------------------------------
# The release packaging in CI builds it in build/release (CALCU_TESTS off);
# the root project's Homebrew prefix covers this build on the Mac.
if(CALCU_FRONTEND_QT)
  find_package(Qt6 QUIET COMPONENTS Widgets)
  if(Qt6_FOUND)
    add_subdirectory(frontends/qt6)
  else()
    message(STATUS "Qt6 not found: building Calc-U-59's core, app layer and tests only")
  endif()
endif()

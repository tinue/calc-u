cmake_minimum_required(VERSION 3.21)

# Calc-U-59 in the repository's root CMake project (option
# CALCU_PRODUCT_59): the emulation core, the app/ C++ layer the frontends
# use, the C++ test suite (CTest, with CALCU_TESTS), the product entry of
# the harness `calcu`, and, with CALCU_FRONTEND_QT and when Qt6 is found, the Qt6
# desktop app via add_subdirectory(frontends/qt6). Under Emscripten only
# the core, the app layer and the Web engine (calcu59_web).
#
# The Swift app links CalcU59.xcframework, which frontends/swift/
# build_xcframework.sh builds from calcu59_core and calcu59_shared of this
# file (and calcu59_harness in the Debug slices with harness attach); Xcode
# compiles only Swift and the bridge. Every core/ or
# app/ change must pass the CMake, Xcode and Web builds — see CLAUDE.md
# § "Frontend Parity".

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "products/calc-u-59 is part of the root CMake project: "
                      "configure the repository root, e.g. cmake -S ../.. -B ../../build/all")
endif()

project(Calc-U-59 LANGUAGES CXX)

# C++20 comes with calcu_library (cxx_std_20, PUBLIC); the
# warnings with calcu_warnings (the root CMakeLists.txt; MSVC's /utf-8 for
# app/'s UTF-8 literals: cue-card math tokens, mnemonics).
set(CORE_ROOT   ${CMAKE_CURRENT_SOURCE_DIR}/core)
set(SHARED_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/app)

# -----------------------------------------------------------------------------
# Core (hardware emulation) and Shared (frontend-neutral logic above it)
# -----------------------------------------------------------------------------
# The core and the TI machine behind the machine interface: TIMachine,
# tiCatalogue() and the adapters; the PC-100C and the Solid State ROM module
# as peripherals.
add_library(calcu59_core STATIC
  ${CORE_ROOT}/PC100C.cpp
  ${CORE_ROOT}/PC100CPaper.cpp
  ${CORE_ROOT}/SolidStateRomModule.cpp
  ${CORE_ROOT}/TI59Machine.cpp
  ${CORE_ROOT}/TMC0501.cpp
  ${CORE_ROOT}/ROM.cpp
  ${CORE_ROOT}/RAM.cpp
  ${CORE_ROOT}/TITypingSupport.cpp
  ${CORE_ROOT}/TIMachineState.cpp
  ${CORE_ROOT}/TIDebugging.cpp
  ${CORE_ROOT}/TIDebugKinds.cpp
  ${CORE_ROOT}/TICatalogue.cpp
  ${CORE_ROOT}/TIKeys.cpp
  ${CORE_ROOT}/TIMachine.cpp
  ${CORE_ROOT}/TIMemoryKinds.cpp
  ${CORE_ROOT}/TIMagneticCard.cpp
  ${CORE_ROOT}/TICardReaderPlace.cpp
  ${CORE_ROOT}/TIRomText.cpp
  ${CORE_ROOT}/TIRomReader.cpp
  ${CORE_ROOT}/ModuleFile.cpp
)
target_include_directories(calcu59_core PUBLIC ${CORE_ROOT})
target_link_libraries(calcu59_core PRIVATE calcu_warnings)
set_target_properties(calcu59_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
# The core and the app layer hold no lock and start no thread; the Swift
# frontend's session thread is the library's (shell/SessionThread).
target_link_libraries(calcu59_core PUBLIC calcu_library)

file(GLOB SHARED_SOURCES CONFIGURE_DEPENDS ${SHARED_ROOT}/*.cpp)
list(REMOVE_ITEM SHARED_SOURCES ${SHARED_ROOT}/HarnessProduct.cpp ${SHARED_ROOT}/TIHarnessCommands.cpp)   # calcu59_harness below
add_library(calcu59_shared STATIC ${SHARED_SOURCES})
target_include_directories(calcu59_shared PUBLIC ${SHARED_ROOT})
target_link_libraries(calcu59_shared PUBLIC calcu59_core)
target_link_libraries(calcu59_shared PRIVATE calcu_warnings)
# Position-independent, as calcu59_core (the XCFramework's libraries).
set_target_properties(calcu59_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# Web engine (emcmake; frontends/web/build.sh copies it into
# frontends/web/dist/wasm/, docs/developer/Web-Client.md): core/ + app/ behind
# frontends/web/bindings.cpp, the factory of the app session the library's
# Web Worker host runs (calcu_web_glue and calcu_web_engine()).
# -fwasm-exceptions: app/ reports ROM and file errors with exceptions;
# native wasm exceptions, as Calc-U-1600's engine and calcu_library use.
# -UNDEBUG: the core's asserts stay on in the Web engine (-O3 without
# -DNDEBUG).
# -----------------------------------------------------------------------------
if(EMSCRIPTEN)
  foreach(target calcu59_core calcu59_shared)
    target_compile_options(${target} PRIVATE -fwasm-exceptions -UNDEBUG)
  endforeach()
  calcu_web_engine(calcu59_web SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/frontends/web/bindings.cpp
                   OUTPUT calcu59-core EXPORT createCalcU59Module ENVIRONMENT web,worker
                   LINK -sNO_EXIT_RUNTIME=1 -sSTACK_SIZE=1MB
                        -sEXPORTED_RUNTIME_METHODS=FS,IDBFS,getExceptionMessage,decrementExceptionRefcount)
  target_link_libraries(calcu59_web PRIVATE calcu59_shared)
  target_compile_options(calcu59_web PRIVATE -UNDEBUG)
  return()
endif()

# The product entry of the harness `calcu` (library/harness/cli/):
# not in calcu59_shared, so the Web build and the Swift Release and device
# builds carry none of it; the Swift Debug builds for macOS and the simulator
# link it for harness attach. It finds the ROMs in the folder the tests use.
add_library(calcu59_harness STATIC ${SHARED_ROOT}/HarnessProduct.cpp ${SHARED_ROOT}/TIHarnessCommands.cpp)
target_link_libraries(calcu59_harness PUBLIC calcu59_shared)
target_link_libraries(calcu59_harness PRIVATE calcu_warnings)
# The state-file header's version is the apps' one: the Xcode project's
# MARKETING_VERSION, as the Qt and Web builds read it.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/frontends/swift/Calc-U-59.xcodeproj/project.pbxproj
     CALCU59_VERSION_LINE REGEX "MARKETING_VERSION = [0-9.]+;" LIMIT_COUNT 1)
string(REGEX REPLACE ".*MARKETING_VERSION = ([0-9.]+).*" "\\1" CALCU59_VERSION "${CALCU59_VERSION_LINE}")
target_compile_definitions(calcu59_harness PRIVATE "CALCU59_ROM_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/roms\""
                                                   "CALCU59_EXAMPLES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/examples\""
                                                   "CALCU59_VERSION=\"${CALCU59_VERSION}\"")
calcu_harness_add_product(calcu59_harness cu59::makeHarnessProduct)

# -----------------------------------------------------------------------------
# Tests (CALCU_TESTS): run from the product directory so they can read roms/
# and examples/
# -----------------------------------------------------------------------------
# The ROM-driven tests run millions of steps; keep Debug builds usable.
calcu_debug_o1(calcu59_core)
if(CALCU_TESTS)
  file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
  add_executable(CalcU59Tests ${TEST_SOURCES})
  # The one test runner: library/testing/, its main() included.
  target_link_libraries(CalcU59Tests PRIVATE calcu59_harness calcu_test_runner calcu_warnings)
  calcu_debug_o1(CalcU59Tests)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)  # scratch files written by the tests
  # Test groups (docs/test-groups.md; calcu_add_test of the root
  # CMakeLists.txt): Frequent always, Longer and Checkpoint only with
  # CALCU_TESTS_LONGER / _CHECKPOINT; every entry runs in this directory.
  calcu_add_test(CalcU59Tests frequent COMMAND CalcU59Tests)
  # tools/read_trace.py must read the trace CalcU59Tests wrote with app/TraceWriter.
  # Its 4 MB of JSON go to /dev/null: CTest took 15 s to take them in;
  # a failure still shows on standard error and in the exit status.
  if(Python3_FOUND)
    calcu_add_test(CalcU59TraceReader frequent
                   COMMAND sh -c "\"$0\" tools/read_trace.py --json build/trace-test.bin > /dev/null" ${Python3_EXECUTABLE}
                   PROPERTIES DEPENDS CalcU59Tests)
  endif()
  # tools/help checks the help and builds Calc-U-59-help.zip into the tree
  # (the test suite builds the asset); the help's own cases are CalcU59Tests'
  # tests/help_tests.cpp.
  calcu_add_help_tests(CalcU59 calc-u-59)
  # The app layer reaches the machine through the interface only.
  calcu_add_test(CalcU59CoreSeam frequent COMMAND sh tools/check_core_seam.sh)
  # Open With offers the three preset extensions on macOS, Windows and Linux.
  calcu_add_test(CalcU59OpenWithTypes frequent COMMAND sh tools/check_open_with_types.sh)
  # No thread in app/ but the session thread; the Swift bridge only through it.
  calcu_add_test(CalcU59SessionThread frequent COMMAND sh tools/check_session_thread.sh)
  # The three frontends draw from the control state.
  calcu_add_test(CalcU59FrontendsDrawControlState frequent COMMAND sh tools/check_frontends_draw_control_state.sh)
  # calcu_product_source_checks (library/):
  # the ROM lookup's allowed files are the product's lookUpRoms(), the
  # catalogue's ROM entries and the Qt6 Bundle.
  calcu_product_source_checks(PREFIX CalcU59
                              ROM_ALLOW app/ProductRoms.cpp app/ProductRoms.hpp core/TICatalogue.cpp
                                        frontends/qt6/app/QtResourceBundle.cpp)
  # `calcu --product c59` runs as a process
  # of its own (tests/calcu_runs.cmake; also in every desktop job of the workflow).
  calcu_add_test(CalcU59CalcuRuns frequent COMMAND ${CMAKE_COMMAND} -DCALCU=$<TARGET_FILE:calcu> -P tests/calcu_runs.cmake)
  if(Python3_FOUND)
    # `calcu` without one ROM file ends naming it.
    calcu_add_test(CalcU59HarnessRomMissing frequent COMMAND ${Python3_EXECUTABLE} tests/harness_rom_tests.py
                   PROPERTIES ENVIRONMENT CALCU=$<TARGET_FILE:calcu>)
    # fastcal.py writes the tracked calendar-09, and
    # years.py runs it for 2026 through calcu (tools/calcu59.py).
    calcu_add_test(CalcU59CalendarScripts frequent
                   COMMAND ${Python3_EXECUTABLE} tools/calendar/selftest.py --calcu $<TARGET_FILE:calcu>)
    # tools/coverage.py and tools/fastmode_matrix.py on the harness
    # (tests/check_scripts.py).
    calcu_add_test(CalcU59Scripts longer
                   COMMAND ${Python3_EXECUTABLE} tests/check_scripts.py --calcu $<TARGET_FILE:calcu>)
  endif()
  # `calcu --product c59 check-preset` over the examples and the startup
  # presets, the ones with magnetic cards included.
  set(CALCU59_PRESET_GLOBS)
  foreach(ext ti59 ti58 ti58c)
    list(APPEND CALCU59_PRESET_GLOBS examples/*.${ext} examples/*/*.${ext})
  endforeach()
  calcu_add_preset_check(CalcU59Presets PRODUCT c59 FILES ${CALCU59_PRESET_GLOBS})
  # The Xcode project compiles only Swift and the bridge.
  calcu_add_test(CalcU59XcodeProjectCheck frequent COMMAND frontends/swift/check_xcode_project.sh)
  # Attach only in Debug builds for macOS and the
  # simulator, in the sources that decide it.
  calcu_add_test(CalcU59NoAttachSources frequent COMMAND sh frontends/swift/check_no_attach.sh --sources)
  # No test aid in the Swift app.
  calcu_add_test(CalcU59NoTestAids frequent COMMAND sh frontends/swift/check_no_test_aids.sh)

  # Longer: run locally before merging into main and at the end of a part.
  # Against frontends/web/dist/ (frontends/web/build.sh first).
  calcu_add_test(CalcU59WebPlaywright longer COMMAND frontends/web/run-tests.sh)
  # Each example through the harness entry against its recorded reference
  # run; a difference is a finding (tests/reference_runs_tests.cpp).
  calcu_add_test(CalcU59ReferenceRuns longer COMMAND CalcU59Tests reference_runs_of_the_examples
                 PROPERTIES ENVIRONMENT CALCU59_REFERENCE_COMPARE=1)
  # A build with one ROM file
  # taken away starts in neither the harness nor the web client, naming it.
  if(NOT WIN32)
    calcu_add_test(CalcU59RomMissingBuild longer COMMAND sh tests/rom_missing_build.sh
                   PROPERTIES ENVIRONMENT CALCU=$<TARGET_FILE:calcu>)
  endif()
  if(APPLE)
    set(iphone "platform=iOS Simulator,id=$(bin/simulators resolve --role iphone)")   # bin/simulators.yaml
    # Every simulator test starts from the known state: the example presets,
    # nothing the app saved (cards pile up otherwise).
    set(reset_iphone "bin/simulators reset --role iphone >/dev/null")
    # XCUITest UIRegressionTests on one simulator (the iphone role).
    calcu_add_test(CalcU59UIRegression longer
                   COMMAND sh -c "${reset_iphone} && xcodebuild test -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59 \
-testPlan UIRegressionTests -destination \"${iphone}\"")
    # The Swift unit tests through the real
    # bridge, in the app on the same simulator.
    calcu_add_test(CalcU59SwiftUnitTests longer
                   COMMAND sh -c "${reset_iphone} && xcodebuild test -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59 \
-testPlan UnitTests -destination \"${iphone}\""
                   PROPERTIES RUN_SERIAL TRUE)
    # The Release Mac app, the Release XCFramework and the
    # Debug iOS device app have no attach; the Release app's entitlements.
    calcu_add_test(CalcU59NoAttachInRelease longer COMMAND sh frontends/swift/check_no_attach.sh
                   PROPERTIES RUN_SERIAL TRUE)
  endif()

  # Checkpoint: run before a release.
  # Pacing on the Web: one minute, against frontends/web/dist/.
  if(NOT WIN32)
    calcu_add_test(CalcU59WebPacing checkpoint COMMAND frontends/web/run-tests.sh tests/pacing.spec.js --project root
                   PROPERTIES ENVIRONMENT CALCU59_WEB_PACING=1 TIMEOUT 300)
  endif()
  calcu_add_test(CalcU59PacingRealClock checkpoint COMMAND CalcU59Tests pacing_longrun_real_clock
                 PROPERTIES ENVIRONMENT CALCU59_PACING_LONGRUN=600 TIMEOUT 900)
  if(APPLE)
    # The App Store set into appstore/screenshots (the iPhone Duo is opened
    # flat by hand) and the XCUITests on macOS.
    calcu_add_test(CalcU59AppStoreScreenshots checkpoint COMMAND bin/run-screenshots)
    calcu_add_test(CalcU59UITestsMac checkpoint
                   COMMAND xcodebuild test -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59
                           -testPlan UIRegressionTests -destination platform=macOS)
    # The Swift unit tests on macOS (macOS's services).
    calcu_add_test(CalcU59SwiftUnitTestsMac checkpoint
                   COMMAND xcodebuild test -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59
                           -testPlan UnitTests -destination platform=macOS
                   PROPERTIES RUN_SERIAL TRUE TIMEOUT 1200)
    # Pacing on the iOS simulator: one minute over attach.
    calcu_add_test(CalcU59PacingIOS checkpoint
                   COMMAND sh -c "${reset_iphone} && xcodebuild test -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59 \
-testPlan PacingCheckpoint -destination \"${iphone}\""
                   PROPERTIES RUN_SERIAL TRUE TIMEOUT 1200)
    # Attach and pacing on the Mac: the Debug app
    # through `calcu attach`; shows a grey window over every screen for 30 s.
    calcu_add_test(CalcU59MacAttach checkpoint COMMAND sh tests/mac_attach_check.sh $<TARGET_FILE:calcu>
                   PROPERTIES RUN_SERIAL TRUE TIMEOUT 1200)
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

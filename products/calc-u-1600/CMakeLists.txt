cmake_minimum_required(VERSION 3.21)

# Calc-U-1600 in the repository's root CMake project (ADR 0003; option
# CALCU_PRODUCT_1600): the calcu_core / calcu_shared libraries
# (cmake/CalcU.cmake), the Qt6 GUI app (with CALCU_FRONTEND_QT, via
# add_subdirectory(frontends/qt6)), the headless CLIs and probes, and the
# core/ + app/ test suite as a CTest target (with CALCU_TESTS). Opening the
# repository root in CLion (or any CMake-aware IDE) gives run/debug
# configurations for all of them -- see the "CLion / IDE Integration"
# section of docs/developer/Building.md. Under Emscripten only the Web
# engine (calcu_web) and the wasm_smoke check (frontends/web/build.sh,
# tools/build_wasm_check.sh; both build in the root's build/web).
#
# CALCU_TESTS_DEBUG (the root's) builds core/ and the tests at -O0 for
# stepping through.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "products/calc-u-1600 is part of the root CMake project (ADR 0003): "
                      "configure the repository root, e.g. cmake -S ../.. -B ../../build/all")
endif()

# The macOS minimum (15.8, libsharpdx) and OBJCXX for the Qt6 app's .mm
# sources are the root project's. No find_package(Qt6) here: only
# frontends/qt6/CMakeLists.txt needs Qt.

set(TOOLS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# The suite runs ~3x faster at -O1 than at -O0, so a Debug build compiles
# Core and the tests at -O1 (calcu_debug_o1; CI builds RelWithDebInfo). To
# step through a failing test, configure a separate full-debug tree:
#   cmake -S ../.. -B ../../build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
#         -DCALCU_FRONTEND_QT=OFF -DCALCU_TESTS_DEBUG=ON
include(cmake/CalcU.cmake)

# -----------------------------------------------------------------------------
# WebAssembly (emcmake cmake on the repository root): only Core + Shared,
# the node smoke test (tools/build_wasm_check.sh) and the web client's
# engine -- no Qt, no CLIs, probes or CoreTests. The smoke test proves the
# frontend-neutral layers build and run for a web GUI.
# -----------------------------------------------------------------------------
if(EMSCRIPTEN)
  # Shared/ reports with exceptions; Core's jobs rethrow through them.
  # Native wasm exception handling, as Rust's wasm32-unknown-emscripten std
  # in libsharpdx-wasm.a uses it (Emscripten's JS-based -fexceptions would
  # leave its __cpp_exception tag unresolved).
  target_compile_options(calcu_core PUBLIC -fwasm-exceptions)
  add_executable(wasm_smoke ${CMAKE_CURRENT_SOURCE_DIR}/tools/wasm_smoke.cpp)
  target_link_libraries(wasm_smoke PRIVATE calcu_shared)
  # NODERAWFS: the smoke test reads roms/ and the bundled cards straight
  # from the host file system, run from the repo root under node.
  target_link_options(wasm_smoke PRIVATE -fwasm-exceptions -sNODERAWFS=1 -sALLOW_MEMORY_GROWTH=1
                      -sENVIRONMENT=node -sEXIT_RUNTIME=1)

  # The web client's engine (frontends/web/build.sh copies it into
  # frontends/web/dist/wasm/): app/ behind frontends/web/bindings.cpp, for
  # the library's Web Worker (calcu_web_engine(), library/web/).
  calcu_web_engine(calcu_web SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/frontends/web/bindings.cpp
                   OUTPUT calcu1600-core EXPORT createCalcU1600Module ENVIRONMENT worker
                   LINK -sSTACK_SIZE=1MB
                        -sEXPORTED_RUNTIME_METHODS=FS,IDBFS,getExceptionMessage,decrementExceptionRefcount)
  target_link_libraries(calcu_web PRIVATE calcu_shared)
  return()
endif()

# -----------------------------------------------------------------------------
# Qt6 GUI Application target
# -----------------------------------------------------------------------------
if(CALCU_FRONTEND_QT)
  add_subdirectory(frontends/qt6)
endif()

# -----------------------------------------------------------------------------
# Headless CLI tools & probes
# -----------------------------------------------------------------------------
# pc1500_cli
add_executable(pc1500_cli ${TOOLS_ROOT}/pc1500_cli.cpp)
target_link_libraries(pc1500_cli PRIVATE calcu1600_harness)

# pc1600_cli
add_executable(pc1600_cli ${TOOLS_ROOT}/pc1600_cli.cpp)
target_link_libraries(pc1600_cli PRIVATE calcu1600_harness)

# pc1600_power_probe, pc1600_uart_probe: the throw-away ROM-path probes.
# They need the instrumentation hooks PC1600_POWER_PROBE lights up in
# PC1600Memory and PC1600SubCpu, so they link a copy of the core built
# with it (calcu_add_core, cmake/CalcU.cmake); no other target defines it.
calcu_add_core(calcu_core_probe)
target_compile_definitions(calcu_core_probe PUBLIC PC1600_POWER_PROBE)

add_executable(pc1600_power_probe ${TOOLS_ROOT}/pc1600_power_probe.cpp)
target_link_libraries(pc1600_power_probe PRIVATE calcu_core_probe)

add_executable(pc1600_uart_probe ${TOOLS_ROOT}/pc1600_uart_probe.cpp)
target_link_libraries(pc1600_uart_probe PRIVATE calcu_core_probe)

# pc1600_plotter_probe
add_executable(pc1600_plotter_probe ${TOOLS_ROOT}/pc1600_plotter_probe.cpp)
target_link_libraries(pc1600_plotter_probe PRIVATE calcu1600_harness)

# The product entry of the harness `calcu` (LIB-11, M-28; library/harness/cli/).
# It finds the ROMs and the module definitions in the folders the tests use
# (LIB-11 design §8).
add_library(calcu1600_harness STATIC ${CMAKE_CURRENT_SOURCE_DIR}/app/HarnessProduct.cpp)
target_link_libraries(calcu1600_harness PUBLIC calcu_shared)
target_link_libraries(calcu1600_harness PRIVATE calcu_warnings)
# The state-file header's version is the app's one: APP_VERSION of the Qt
# build (user, 2026-11-15).
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/frontends/qt6/CMakeLists.txt
     CALCU1600_VERSION_LINE REGEX "^set\\(APP_VERSION \"[0-9.]+\"\\)" LIMIT_COUNT 1)
string(REGEX REPLACE ".*\"([0-9.]+)\".*" "\\1" CALCU1600_VERSION "${CALCU1600_VERSION_LINE}")
target_compile_definitions(calcu1600_harness PRIVATE "CALCU1600_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\""
                                                     "CALCU1600_VERSION=\"${CALCU1600_VERSION}\"")
calcu_harness_add_product(calcu1600_harness shared::makeHarnessProduct)

# -----------------------------------------------------------------------------
# Test Suite target (CALCU_TESTS; the release packaging turns it off, which
# also spares the Windows runners a zlib)
# -----------------------------------------------------------------------------
if(CALCU_TESTS)
  find_package(ZLIB REQUIRED)

  file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/app/tests/*.cpp
  )

  add_executable(CoreTests ${TEST_SOURCES})
  # The one test runner (ADR 0038): library/testing/, its main() included.
  target_link_libraries(CoreTests PRIVATE calcu1600_harness calcu_test_runner calcu_warnings ZLIB::ZLIB)
  calcu_debug_o1(CoreTests)

  # Test groups (ADR 0031, docs/test-groups.md; calcu_add_test of the root
  # CMakeLists.txt): Frequent always, Longer and Checkpoint only with
  # CALCU_TESTS_LONGER / _CHECKPOINT; every entry runs in this directory.
  calcu_add_test(CoreTests frequent COMMAND CoreTests)
  # LIB-6 AC-51 (design §10.6 P8; C1600-4 T7): every preset of the
  # repository is read by the library's reader through `calcu check-preset`;
  # since M-47 (LIB-7 1600) with the six that waited for the media. The
  # VS Code templates are checked filled in.
  set(CALCU1600_PRESET_GLOBS)
  foreach(ext pc1500 pc1500a pc1600)
    list(APPEND CALCU1600_PRESET_GLOBS examples/*/*.${ext} examples/*/*/*.${ext} tests/fixtures/presets/*.${ext}
         dev/hardware-checks/*.${ext} dev/presets/*.${ext} docs/developer/screenshots/presets/*.${ext}
         vscode/calcu1600-debug/presets/*.${ext})
  endforeach()
  calcu_add_preset_check(CalcU1600Presets PRODUCT 1600 FILES ${CALCU1600_PRESET_GLOBS})
  calcu_add_preset_check(CalcU1600Templates PRODUCT 1600 FILES vscode/calcu1600-debug/templates/*/debug.*
                         FILL tests/fill_vscode_templates.cmake)
  # No file above the core includes or names a machine class (C1600-1,
  # Technical Requirements; design T13). CI runs it with the core tests.
  calcu_add_test(CalcU1600MachineIncludeCheck frequent COMMAND sh tools/check_machine_includes.sh)
  # LIB-12 AC-2, LIB-9 AC-31 (calcu_product_source_checks, library/): the ROM
  # lookup's allowed files are the host drive's own ROM
  # (core/Machine/SharpRoms.cpp; D-6) and the catalogue's ROM entries.
  calcu_product_source_checks(PREFIX CalcU1600 ROM_ALLOW core/Machine/SharpRoms.cpp core/Machine/SharpCatalogue.cpp)
  # LIB-3 AC-34, AC-36 (design §6 P16-1, P16-3): no session class of the
  # product's own; the Qt6 frontend hosts the library's app session and
  # sends no suspend.
  calcu_add_test(CalcU1600NoOwnSession frequent COMMAND ${CMAKE_COMMAND} -P tools/check_no_own_session.cmake)
  # LIB-13 AC-21 (design Part D, T16-5): the macOS Open With types: the
  # preset extensions as the product's own, .bas, .bbin, .bin as Alternate,
  # no .rom.
  calcu_add_test(CalcU1600OpenWithTypes frequent COMMAND sh tools/check_open_with_types.sh)
  # LIB-12 AC-13 (design Q2): pc1600_cli with a roms/ that lacks one file
  # exits 1 naming it; with the full set it runs.
  calcu_add_test(CalcU1600CliRomLookup frequent COMMAND sh tests/cli_rom_tests.sh
                 PROPERTIES ENVIRONMENT PC1600_CLI=$<TARGET_FILE:pc1600_cli>)
  # LIB-10 AC-42 (design §10.10, D16-8): the harness writes a PC-1600 trace,
  # tools/read_trace.py reads it unchanged (well under 2 s: Frequent).
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND AND NOT WIN32)
    calcu_add_test(CalcU1600ReadTrace frequent COMMAND sh tests/read_trace_check.sh
                   PROPERTIES ENVIRONMENT CALCU=$<TARGET_FILE:calcu>)
  endif()

  # Longer: run locally before merging into main and at the end of a part.
  # Against frontends/web/dist/ (frontends/web/build.sh first); the WASM
  # smoke builds in build/web itself (Emscripten).
  calcu_add_test(CalcU1600WebPlaywright longer COMMAND frontends/web/run-tests.sh)
  calcu_add_test(CalcU1600WasmSmoke longer COMMAND tools/build_wasm_check.sh)
  # C1600-4 AC-16 (design §6 T8): the examples against their reference runs.
  calcu_add_test(CalcU1600ExampleReferences longer COMMAND CoreTests c1600_4_ac16
                 PROPERTIES ENVIRONMENT CALCU1600_REFERENCES=1)
  # LIB-10 R-1 (design §10.11): full speed with a breakpoint set, printed for the qa-report.
  calcu_add_test(CalcU1600DebuggerSpeed longer COMMAND CoreTests lib10_speed_pc1600_full_speed_with_a_breakpoint
                 PROPERTIES ENVIRONMENT CALCU1600_SPEED=1)
  # LIB-12 (Technical Requirements, design Q5): a Web dist/ built from a
  # ROM folder without one file names it and starts no calculator.
  if(NOT WIN32)
    calcu_add_test(CalcU1600RomMissingBuild longer COMMAND sh tests/rom_missing_build.sh PROPERTIES RUN_SERIAL TRUE)
  endif()
  if(TARGET CalcU1600Qt)
    # The screenshot scenarios run as checks: every shot is taken, into
    # the build tree, not docs/images/.
    # The check scenarios of the GUI criteria (tests/gui/, ADR 0032: LIB-8
    # AC-36, AC-38, LIB-10 AC-41) run the same way, under their IDs.
    file(GLOB CALCU1600_SCENARIOS ${CMAKE_CURRENT_SOURCE_DIR}/docs/developer/screenshots/guide/*.shots.yaml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/tests/gui/*.shots.yaml)
    foreach(scenario ${CALCU1600_SCENARIOS})
      get_filename_component(name ${scenario} NAME_WE)
      # LIB-5 AC-33 (design T16-4, the design's CalcU1600QtTyping) sends
      # host key events to the window: offscreen, no window shown.
      set(shot_environment)
      if(name STREQUAL "lib5-ac33")
        set(shot_environment ENVIRONMENT QT_QPA_PLATFORM=offscreen)
      endif()
      calcu_add_test(CalcU1600Shots.${name} longer
                     COMMAND CalcU1600Qt --shots ${scenario} --shots-out ${CMAKE_CURRENT_BINARY_DIR}/shots-check/${name}
                     PROPERTIES RUN_SERIAL TRUE ${shot_environment})
    endforeach()
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
      calcu_add_test(CalcU1600DapSmoke longer
                     COMMAND ${Python3_EXECUTABLE} tools/dap_smoke.py --app $<TARGET_FILE:CalcU1600Qt>
                     PROPERTIES RUN_SERIAL TRUE)
    endif()
  endif()

  # Checkpoint: run before a release.
  # Ten minutes per model on the real clock (PC-1500A, then PC-1600).
  calcu_add_test(CalcU1600PacingRealClock checkpoint COMMAND CoreTests pacing_longrun_real_clock
                 PROPERTIES ENVIRONMENT CALCU1600_PACING_LONGRUN=600 TIMEOUT 1500)
  if(TARGET CalcU1600Qt)
    # LIB-17 AC-22 (design §6 P16-3): a PC-1600 keeps one minute in the Qt6
    # frontend (the --pacing-probe test hook of non-release builds).
    calcu_add_test(CalcU1600QtPacingProbe checkpoint COMMAND CalcU1600Qt --pacing-probe 60
                   PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
    # The User Guide screenshots into docs/images/ (tools/make_screenshots.sh
    # builds the app in build/app-1600).
    calcu_add_test(CalcU1600GuideScreenshots checkpoint COMMAND tools/make_screenshots.sh PROPERTIES RUN_SERIAL TRUE)
  endif()
endif()

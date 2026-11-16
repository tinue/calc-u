cmake_minimum_required(VERSION 3.21)

# Calc-U-1600 in the repository's root CMake project (ADR 0003; option
# CALCU_PRODUCT_1600): the calcu_core / calcu_shared libraries
# (cmake/CalcU.cmake), the Qt6 GUI app (with CALCU_FRONTEND_QT, via
# add_subdirectory(frontends/qt6)), the product's entry of the harness
# `calcu` (LIB-11; the CLIs and probes went in M-51), and the core/ + app/
# test suite as CTest targets (with CALCU_TESTS). Opening the
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
# engine -- no Qt, no harness, no CoreTests. The smoke test proves the
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

# The product entry of the harness `calcu` (LIB-11, M-28, M-51; library/harness/cli/).
# It finds the ROMs and the module definitions in the folders of the source
# tree, as the tests use them (LIB-11 design §8, AC-20).
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
  # LIB-11 Part D, D.6 H4 (M-51): the CLIs, the probes, the one-time checks
  # and their run configurations are gone, and no PC1600_POWER_PROBE is left.
  calcu_add_test(CalcU1600NoOldTools frequent COMMAND ${CMAKE_COMMAND} -P tools/check_no_old_tools.cmake)
  # LIB-10 AC-42 (design §10.10, D16-8): the harness writes a PC-1600 trace,
  # tools/read_trace.py reads it unchanged (well under 2 s: Frequent).
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND AND NOT WIN32)
    calcu_add_test(CalcU1600ReadTrace frequent COMMAND sh tests/read_trace_check.sh
                   PROPERTIES ENVIRONMENT CALCU=$<TARGET_FILE:calcu>)
  endif()

  # C1600-3 (design §6): the DAP server's cases, a program of their own so
  # that CoreTests' Frequent budget stays as it is (tests/dap/).
  file(GLOB DAP_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/dap/*.cpp)
  add_executable(CalcU1600DapTests ${DAP_TEST_SOURCES})
  target_include_directories(CalcU1600DapTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_SOURCE_DIR}/app/tests)
  target_link_libraries(CalcU1600DapTests PRIVATE calcu1600_harness calcu_test_runner calcu_warnings)
  calcu_debug_o1(CalcU1600DapTests)
  calcu_add_test(CalcU1600DapTests frequent COMMAND CalcU1600DapTests)
  # EC-1 under the harness (D-6): `serve --dap 80` ends with exit status 2.
  if(NOT WIN32)
    calcu_add_test(CalcU1600DapPortCheck frequent COMMAND sh tests/dap/dap_port_check.sh
                   PROPERTIES ENVIRONMENT CALCU=$<TARGET_FILE:calcu>)
  endif()
  # C1600-3 AC-20, AC-22 (T14): the scenarios of tools/dap_smoke.py against
  # `calcu --product 1600 serve --dap` (no window; ~17 s with its pauses by hand:
  # Longer, D-5). Where attach is built (D-3).
  if(Python3_FOUND AND NOT WIN32 AND CALCU_ATTACH)
    calcu_add_test(CalcU1600DapSmoke longer
                   COMMAND ${Python3_EXECUTABLE} tools/dap_smoke.py --port 32170 --harness $<TARGET_FILE:calcu>
                   PROPERTIES RUN_SERIAL TRUE)
  endif()

  # LIB-16 (design §2.1, §3.5): the help. CalcU1600HelpTests compares
  # help/settings-keys.txt with the declaration and help.yaml's names with
  # the catalogue (a program of its own, so that CoreTests' budget stays);
  # tools/help checks the help and builds Calc-U-1600-help.zip into the
  # tree (the test suite builds the asset, AC-8). Python 3, standard library.
  add_executable(CalcU1600HelpTests ${CMAKE_CURRENT_SOURCE_DIR}/tests/help/help_tests.cpp)
  target_link_libraries(CalcU1600HelpTests PRIVATE calcu1600_harness calcu_test_runner calcu_warnings)
  calcu_add_test(CalcU1600HelpTests frequent COMMAND CalcU1600HelpTests)
  if(Python3_FOUND)
    calcu_add_test(CalcU1600HelpCheck frequent
                   COMMAND ${Python3_EXECUTABLE} -I -B ${CMAKE_SOURCE_DIR}/tools/help check calc-u-1600 --quiet-warnings)
    calcu_add_test(CalcU1600HelpBuild frequent
                   COMMAND ${Python3_EXECUTABLE} -I -B ${CMAKE_SOURCE_DIR}/tools/help build calc-u-1600 --quiet-warnings
                           --out ${CMAKE_CURRENT_BINARY_DIR}/help)
  endif()

  # C1600-5 (design §6) and LIB-11 Part D (D.6): the product commands under
  # `pc1600` and the harness entry through SharpAppPart, each test on a
  # `calcu` run of its own; a program of its own, so that CoreTests' budget
  # stays (tests/harness/). About 10 s: Longer.
  file(GLOB HARNESS_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/harness/*.cpp)
  add_executable(CalcU1600HarnessTests ${HARNESS_TEST_SOURCES})
  target_include_directories(CalcU1600HarnessTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_link_libraries(CalcU1600HarnessTests PRIVATE calcu1600_harness calcu_test_runner calcu_warnings)
  calcu_debug_o1(CalcU1600HarnessTests)
  calcu_add_test(CalcU1600HarnessTests longer COMMAND CalcU1600HarnessTests)

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
    # The screenshot scenarios (one per help chapter) run as checks: every
    # shot is taken, into the build tree, not help/images/.
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
      # C1600-3 AC-18: the Debugger section with the server listening.
      set(shot_arguments)
      if(name STREQUAL "c1600-3-ac18")
        set(shot_arguments --dap 32177)
      endif()
      calcu_add_test(CalcU1600Shots.${name} longer
                     COMMAND CalcU1600Qt --shots ${scenario} --shots-out ${CMAKE_CURRENT_BINARY_DIR}/shots-check/${name} ${shot_arguments}
                     PROPERTIES RUN_SERIAL TRUE ${shot_environment})
    endforeach()
    # C1600-3 T15: the scenarios of tools/dap_smoke.py against the Qt app
    # (and M-50's exit check, its vscode_session).
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
      calcu_add_test(CalcU1600DapSmokeQt longer
                     COMMAND ${Python3_EXECUTABLE} tools/dap_smoke.py --port 32169 --app $<TARGET_FILE:CalcU1600Qt>
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
    # LIB-16 AC-12: the help's pictures into help/images/, one scenario per
    # help chapter (tools/make_screenshots.sh builds the app in build/app-1600).
    calcu_add_test(CalcU1600HelpPictures checkpoint COMMAND tools/make_screenshots.sh PROPERTIES RUN_SERIAL TRUE)
  endif()
endif()

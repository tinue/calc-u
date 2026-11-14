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
# Options (besides the root ones):
#   CALCU_TESTS_DEBUG  build core/ and the tests at -O0 for stepping through

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "products/calc-u-1600 is part of the root CMake project (ADR 0003): "
                      "configure the repository root, e.g. cmake -S ../.. -B ../../build/all")
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET "15.8" CACHE STRING "Minimum macOS version")

if(APPLE)
  project(Calc-U-1600 LANGUAGES CXX OBJCXX)
else()
  project(Calc-U-1600 LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# No find_package(Qt6) here: only frontends/qt6/CMakeLists.txt needs Qt.

set(TOOLS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# The suite runs ~3x faster at -O1 than at -O0, so a Debug build compiles
# Core and the tests at -O1 (CI builds RelWithDebInfo). To step through a
# failing test, configure a separate full-debug tree:
#   cmake -S ../.. -B ../../build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
#         -DCALCU_FRONTEND_QT=OFF -DCALCU_TESTS_DEBUG=ON
option(CALCU_TESTS_DEBUG "Build Core and CoreTests at -O0 for debugging" OFF)
if(NOT CALCU_TESTS_DEBUG)
  set(CALCU_CORE_DEBUG_O1 ON)
endif()
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
  # frontends/web/dist/wasm/):
  # Shared/ behind Web/bindings.cpp, for a Web Worker. No -pthread: a static
  # host can't be relied on for the COOP/COEP headers threads need. IDBFS
  # keeps the save folder in IndexedDB (mounted by the worker).
  add_executable(calcu_web ${CMAKE_CURRENT_SOURCE_DIR}/frontends/web/bindings.cpp)
  target_link_libraries(calcu_web PRIVATE calcu_shared)
  set_target_properties(calcu_web PROPERTIES OUTPUT_NAME calcu1600-core)
  target_link_options(calcu_web PRIVATE -fwasm-exceptions -lembind -lidbfs.js -sMODULARIZE=1
                      -sEXPORT_NAME=createCalcU1600Module -sENVIRONMENT=worker -sALLOW_MEMORY_GROWTH=1
                      -sSTACK_SIZE=1MB
                      -sEXPORTED_RUNTIME_METHODS=FS,IDBFS,getExceptionMessage,decrementExceptionRefcount)
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
target_link_libraries(pc1500_cli PRIVATE calcu_core)

# pc1600_cli
add_executable(pc1600_cli ${TOOLS_ROOT}/pc1600_cli.cpp)
target_link_libraries(pc1600_cli PRIVATE calcu_core)

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
target_link_libraries(pc1600_plotter_probe PRIVATE calcu_core)

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
  target_link_libraries(CoreTests PRIVATE calcu_shared calcu_test_runner ZLIB::ZLIB)
  target_compile_options(CoreTests PRIVATE ${CALCU_WARNINGS})
  if(NOT CALCU_TESTS_DEBUG AND NOT MSVC)
    target_compile_options(CoreTests PRIVATE $<$<CONFIG:Debug>:-O1>)
  endif()

  enable_testing()
  # Test groups (ADR 0031, docs/test-groups.md): labels frequent, longer,
  # checkpoint; the last two only with CALCU_TESTS_LONGER / _CHECKPOINT.
  add_test(NAME CoreTests COMMAND CoreTests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  set_tests_properties(CoreTests PROPERTIES LABELS frequent)
  # Every preset in the repository parses (nothing is booted).
  add_test(NAME CalcU1600CheckPresets COMMAND tools/check_presets.sh
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  set_tests_properties(CalcU1600CheckPresets PROPERTIES LABELS frequent
                       ENVIRONMENT PC1600_CLI=$<TARGET_FILE:pc1600_cli>)

  # Longer: run locally before merging into main and at the end of a part.
  if(CALCU_TESTS_LONGER)
    # Against frontends/web/dist/ (frontends/web/build.sh first); the WASM
    # smoke builds in build/web itself (Emscripten).
    add_test(NAME CalcU1600WebPlaywright COMMAND frontends/web/run-tests.sh
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME CalcU1600WasmSmoke COMMAND tools/build_wasm_check.sh
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(CalcU1600WebPlaywright CalcU1600WasmSmoke PROPERTIES LABELS longer)
    if(TARGET CalcU1600Qt)
      # The screenshot scenarios run as checks: every shot is taken, into
      # the build tree, not docs/images/.
      file(GLOB CALCU1600_SCENARIOS ${CMAKE_CURRENT_SOURCE_DIR}/docs/developer/screenshots/guide/*.shots.yaml)
      foreach(scenario ${CALCU1600_SCENARIOS})
        get_filename_component(name ${scenario} NAME_WE)
        add_test(NAME CalcU1600Shots.${name}
                 COMMAND CalcU1600Qt --shots ${scenario} --shots-out ${CMAKE_CURRENT_BINARY_DIR}/shots-check/${name}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        set_tests_properties(CalcU1600Shots.${name} PROPERTIES LABELS longer RUN_SERIAL TRUE)
      endforeach()
      find_package(Python3 COMPONENTS Interpreter)
      if(Python3_FOUND)
        add_test(NAME CalcU1600DapSmoke
                 COMMAND ${Python3_EXECUTABLE} tools/dap_smoke.py --app $<TARGET_FILE:CalcU1600Qt>
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        set_tests_properties(CalcU1600DapSmoke PROPERTIES LABELS longer RUN_SERIAL TRUE)
      endif()
    endif()
  endif()

  # Checkpoint: run before a release.
  if(CALCU_TESTS_CHECKPOINT)
    # Ten minutes per model on the real clock (PC-1500A, then PC-1600).
    add_test(NAME CalcU1600PacingRealClock COMMAND CoreTests pacing_longrun_real_clock
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(CalcU1600PacingRealClock PROPERTIES LABELS checkpoint
                         ENVIRONMENT CALCU1600_PACING_LONGRUN=600 TIMEOUT 1500)
    if(TARGET CalcU1600Qt)
      # The User Guide screenshots into docs/images/ (tools/make_screenshots.sh
      # builds the app in build/app-1600).
      add_test(NAME CalcU1600GuideScreenshots COMMAND tools/make_screenshots.sh
               WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      set_tests_properties(CalcU1600GuideScreenshots PROPERTIES LABELS checkpoint RUN_SERIAL TRUE)
    endif()
  endif()
endif()

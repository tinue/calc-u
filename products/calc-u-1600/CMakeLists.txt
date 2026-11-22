cmake_minimum_required(VERSION 3.21)

# Calc-U-1600 in the repository's root CMake project (option
# CALCU_PRODUCT_1600): the calcu1600_core / calcu1600_shared libraries,
# the Qt6 GUI app (with CALCU_FRONTEND_QT, via
# add_subdirectory(frontends/qt6)), the product's entry of the harness
# `calcu`, and the core/ + app/
# test suite as CTest targets (with CALCU_TESTS). Opening the
# repository root in CLion (or any CMake-aware IDE) gives run/debug
# configurations for all of them -- see the "CLion and other IDEs"
# section of the repository's docs/Building.md. Under Emscripten only the Web
# engine (calcu1600_web) and the wasm_smoke check (frontends/web/build.sh,
# tools/build_wasm_check.sh; both build in the root's build/web).
#
# CALCU_TESTS_DEBUG (the root's) builds core/ and the tests at -O0 for
# stepping through.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "products/calc-u-1600 is part of the root CMake project: "
                      "configure the repository root, e.g. cmake -S ../.. -B ../../build/all")
endif()

# The macOS minimum (15.8, libsharpdx) and OBJCXX for the Qt6 app's .mm
# sources are the root project's. No find_package(Qt6) here: only
# frontends/qt6/CMakeLists.txt needs Qt.

# The suite runs ~3x faster at -O1 than at -O0, so a Debug build compiles
# Core and the tests at -O1 (calcu_debug_o1; CI builds RelWithDebInfo). To
# step through a failing test, configure a separate full-debug tree:
#   cmake -S ../.. -B ../../build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
#         -DCALCU_FRONTEND_QT=OFF -DCALCU_TESTS_DEBUG=ON

# -----------------------------------------------------------------------------
# The emulation core and the frontend-neutral app layer, two static
# libraries for every tree:
#
#   calcu1600_core    core/ -- the hardware (CPUs, machines, cards, loaders)
#   calcu1600_shared  app/  -- the app layer: everything above the core that
#                     isn't drawing (docs/developer/App-Layer.md); links
#                     calcu1600_core
#
# The source lists are globbed: every core/**/*.cpp and every app/**/*.cpp
# except app/tests/. calcu1600_core is compiled at -O1 in Debug unless
# CALCU_TESTS_DEBUG is ON (calcu_debug_o1, the root CMakeLists.txt).
# -----------------------------------------------------------------------------
set(CORE_ROOT   ${CMAKE_CURRENT_SOURCE_DIR}/core)
set(SHARED_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/app)

# ---- libsharpdx: every program loader needs it (core/Basic/vendor/sharpdx/README.md)
#  - macOS: libsharpdx.a is committed (tools/refresh_sharpdx.sh).
#  - Linux/Windows: fetched by tools/fetch_sharpdx.sh into libsharpdx-linux.a
#    / sharpdx.lib -- different names, so a fetch never dirties the
#    committed macOS binary.
#  - Emscripten: libsharpdx-wasm.a (tools/build_sharpdx_wasm.sh).
if(EMSCRIPTEN)
  set(SHARPDX_LIB ${CORE_ROOT}/Basic/vendor/sharpdx/libsharpdx-wasm.a)
elseif(WIN32)
  set(SHARPDX_LIB ${CORE_ROOT}/Basic/vendor/sharpdx/sharpdx.lib)
elseif(APPLE)
  set(SHARPDX_LIB ${CORE_ROOT}/Basic/vendor/sharpdx/libsharpdx.a)
else()
  set(SHARPDX_LIB ${CORE_ROOT}/Basic/vendor/sharpdx/libsharpdx-linux.a)
endif()
if(NOT EXISTS ${SHARPDX_LIB})
  message(FATAL_ERROR "${SHARPDX_LIB} is missing -- run tools/fetch_sharpdx.sh (Linux/Windows), "
                      "tools/refresh_sharpdx.sh (macOS) or tools/build_sharpdx_wasm.sh (Emscripten)")
endif()

add_library(calcu_sharpdx INTERFACE)
target_link_libraries(calcu_sharpdx INTERFACE ${SHARPDX_LIB})
# The vendored/fetched libs are sharpdx's serial-free build, so there's no
# libudev (Linux), IOKit (macOS) or serialport import lib (Windows) to link
# -- only Rust std's own Windows system libs, below.
if(WIN32)
  # `cargo build` links the Windows system import libs Rust's std needs
  # (sockets, CSPRNG seeding, futex-based sync, ...) into sde.exe
  # automatically; consuming the raw sharpdx.lib from CMake doesn't, so
  # without this the link fails with unresolved externals such as
  # __imp_freeaddrinfo (ws2_32). The authoritative list is the one rustc
  # reported for that exact build: SharpDataExchange's release packaging
  # captures `rustc --print=native-static-libs` into
  # native-libs-windows.txt and tools/fetch_sharpdx.sh vendors it next to
  # sharpdx.lib. Only a hand-placed sharpdx.lib without that file falls
  # back to the hardcoded guess.
  #
  # On *-pc-windows-msvc rustc's note is MSVC linker syntax (bare
  # `name.lib` names and the odd `/defaultlib:x`), not `-lname`, so it goes
  # in as raw link options: target_link_libraries would look for a library
  # literally named "/defaultlib:msvcrt" and fail.
  set(SHARPDX_NATIVE_LIBS_FILE ${CORE_ROOT}/Basic/vendor/sharpdx/native-libs-windows.txt)
  if(EXISTS ${SHARPDX_NATIVE_LIBS_FILE})
    file(READ ${SHARPDX_NATIVE_LIBS_FILE} SHARPDX_NATIVE_LIBS_RAW)
    string(STRIP "${SHARPDX_NATIVE_LIBS_RAW}" SHARPDX_NATIVE_LIBS_RAW)
    separate_arguments(SHARPDX_NATIVE_LINK_OPTS UNIX_COMMAND "${SHARPDX_NATIVE_LIBS_RAW}")
    message(STATUS "sharpdx: linking native libs from native-libs-windows.txt: ${SHARPDX_NATIVE_LINK_OPTS}")
    target_link_options(calcu_sharpdx INTERFACE ${SHARPDX_NATIVE_LINK_OPTS})
  else()
    set(SHARPDX_NATIVE_LIBS_FALLBACK ws2_32 userenv advapi32 bcrypt ntdll synchronization)
    message(STATUS "sharpdx: native-libs-windows.txt not found, using hardcoded fallback: ${SHARPDX_NATIVE_LIBS_FALLBACK}")
    target_link_libraries(calcu_sharpdx INTERFACE ${SHARPDX_NATIVE_LIBS_FALLBACK})
  endif()
endif()

# ---- calcu1600_core
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS ${CORE_ROOT}/*.cpp)
add_library(calcu1600_core STATIC ${CORE_SOURCES})
target_include_directories(calcu1600_core PUBLIC ${CORE_ROOT})
target_compile_features(calcu1600_core PUBLIC cxx_std_20)
target_link_libraries(calcu1600_core PRIVATE calcu_warnings)
# The sources are UTF-8 without a BOM (U'á' literals, PC1600TypedInput.hpp);
# MSVC would read them in the ANSI codepage (C2015). Targets linking Qt got
# /utf-8 from Qt6::Platform; core/ and app/ don't link Qt, so set it here --
# PUBLIC, since everything that includes core/'s headers needs it too.
if(MSVC)
  target_compile_options(calcu1600_core PUBLIC /utf-8)
endif()
target_link_libraries(calcu1600_core PUBLIC calcu_sharpdx)
# The library (the root project's calcu_library): the machine interface,
# the settings file. Products do not configure on their own.
target_link_libraries(calcu1600_core PUBLIC calcu_library)
calcu_debug_o1(calcu1600_core)

# ---- calcu1600_shared
# app/ is C++20 and standard-library-only: no Qt, Cocoa or Emscripten
# headers, and single-threaded (docs/developer/App-Layer.md).
# Headers include their core dependencies relative to core/ ("PC1600/...")
# and each other as "app/...".
file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS ${SHARED_ROOT}/*.cpp)
list(FILTER SHARED_SOURCES EXCLUDE REGEX "^${SHARED_ROOT}/tests/")
# The harness product entry is a target of its own:
# calcu1600_harness below, not in the Qt6 and Web builds.
list(FILTER SHARED_SOURCES EXCLUDE REGEX "/app/HarnessProduct\\.cpp$")
# The DAP's layer: every tree but Emscripten -- the Web has
# no debugger client.
if(EMSCRIPTEN)
  list(FILTER SHARED_SOURCES EXCLUDE REGEX "/app/dap/")
endif()
add_library(calcu1600_shared STATIC ${SHARED_SOURCES})
# The Web page's test-only commands (library/CMakeLists.txt):
# a definition of the one file that holds them, so that switching the
# option compiles only it again.
if(CALCU_WEB_TEST_AIDS)
  set_source_files_properties(${SHARED_ROOT}/SharpWebPart.cpp PROPERTIES COMPILE_DEFINITIONS CALCU_WEB_TEST_AIDS)
endif()
target_include_directories(calcu1600_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(calcu1600_shared PRIVATE calcu_warnings)
target_link_libraries(calcu1600_shared PUBLIC calcu1600_core)

# -----------------------------------------------------------------------------
# WebAssembly (emcmake cmake on the repository root): only core/ + app/,
# the node smoke test (tools/build_wasm_check.sh) and the web client's
# engine -- no Qt, no harness, no CalcU1600CoreTests. The smoke test proves the
# frontend-neutral layers build and run for a web GUI.
# -----------------------------------------------------------------------------
if(EMSCRIPTEN)
  # app/ reports with exceptions; the core's jobs rethrow through them.
  # Native wasm exception handling, as Rust's wasm32-unknown-emscripten std
  # in libsharpdx-wasm.a uses it (Emscripten's JS-based -fexceptions would
  # leave its __cpp_exception tag unresolved).
  target_compile_options(calcu1600_core PUBLIC -fwasm-exceptions)
  add_executable(wasm_smoke ${CMAKE_CURRENT_SOURCE_DIR}/tools/wasm_smoke.cpp)
  target_link_libraries(wasm_smoke PRIVATE calcu1600_shared)
  # NODERAWFS: the smoke test reads roms/ and the bundled cards straight
  # from the host file system, run from the repo root under node.
  target_link_options(wasm_smoke PRIVATE -fwasm-exceptions -sNODERAWFS=1 -sALLOW_MEMORY_GROWTH=1
                      -sENVIRONMENT=node -sEXIT_RUNTIME=1)

  # The web client's engine (frontends/web/build.sh copies it into
  # frontends/web/dist/wasm/): app/ behind frontends/web/bindings.cpp, for
  # the library's Web Worker (calcu_web_engine(), library/web/).
  calcu_web_engine(calcu1600_web SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/frontends/web/bindings.cpp
                   OUTPUT calcu1600-core EXPORT createCalcU1600Module ENVIRONMENT worker
                   LINK -sSTACK_SIZE=1MB
                        -sEXPORTED_RUNTIME_METHODS=FS,IDBFS,getExceptionMessage,decrementExceptionRefcount)
  target_link_libraries(calcu1600_web PRIVATE calcu1600_shared)
  return()
endif()

# -----------------------------------------------------------------------------
# Qt6 GUI Application target
# -----------------------------------------------------------------------------
if(CALCU_FRONTEND_QT)
  add_subdirectory(frontends/qt6)
endif()

# The product entry of the harness `calcu` (library/harness/cli/).
# It finds the ROMs and the module definitions in the folders of the source
# tree, as the tests use them.
add_library(calcu1600_harness STATIC ${CMAKE_CURRENT_SOURCE_DIR}/app/HarnessProduct.cpp)
target_link_libraries(calcu1600_harness PUBLIC calcu1600_shared)
target_link_libraries(calcu1600_harness PRIVATE calcu_warnings)
# The state-file header's version is the app's one: APP_VERSION of the Qt
# build.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/frontends/qt6/CMakeLists.txt
     CALCU1600_VERSION_LINE REGEX "^set\\(APP_VERSION \"[0-9.]+\"\\)" LIMIT_COUNT 1)
string(REGEX REPLACE ".*\"([0-9.]+)\".*" "\\1" CALCU1600_VERSION "${CALCU1600_VERSION_LINE}")
target_compile_definitions(calcu1600_harness PRIVATE "CALCU1600_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\""
                                                     "CALCU1600_VERSION=\"${CALCU1600_VERSION}\"")
calcu_harness_add_product(calcu1600_harness cu1600::makeHarnessProduct)

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
  # The machine state's tests have a program of their own (below).
  list(REMOVE_ITEM TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/machine_state_tests.cpp)

  # calcu1600_test_program(<name> <sources>…): a test program on the
  # harness entry and the one test runner (library/testing/, its
  # main() included), with tests/ and app/tests/ on its include path.
  function(calcu1600_test_program name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_SOURCE_DIR}/app/tests)
    target_link_libraries(${name} PRIVATE calcu1600_harness calcu_test_runner calcu_warnings)
    calcu_debug_o1(${name})
  endfunction()

  # The tests on an app session (app/tests/SessionFixture.hpp: a session
  # and a cold boot per test, about half of the suite's time) are two
  # programs of their own, those of tests/ and those of app/tests/, so that
  # ctest runs the three side by side (the Frequent budget,
  # docs/test-groups.md); the support sources go into each.
  set(SESSION_TEST_SOURCES)
  set(APP_SESSION_TEST_SOURCES)
  set(SUPPORT_TEST_SOURCES)
  foreach(f ${TEST_SOURCES})
    if(NOT f MATCHES "_tests\\.cpp$")
      list(APPEND SUPPORT_TEST_SOURCES ${f})
      continue()
    endif()
    file(STRINGS ${f} uses REGEX "SessionFixture" LIMIT_COUNT 1)
    if(uses AND f MATCHES "/app/tests/")
      list(APPEND APP_SESSION_TEST_SOURCES ${f})
    elseif(uses)
      list(APPEND SESSION_TEST_SOURCES ${f})
    endif()
  endforeach()
  list(REMOVE_ITEM TEST_SOURCES ${SESSION_TEST_SOURCES} ${APP_SESSION_TEST_SOURCES})

  calcu1600_test_program(CalcU1600CoreTests ${TEST_SOURCES})
  target_link_libraries(CalcU1600CoreTests PRIVATE ZLIB::ZLIB)
  calcu1600_test_program(CalcU1600SessionTests ${SESSION_TEST_SOURCES} ${SUPPORT_TEST_SOURCES})
  calcu1600_test_program(CalcU1600AppSessionTests ${APP_SESSION_TEST_SOURCES} ${SUPPORT_TEST_SOURCES})

  # Test groups (docs/test-groups.md; calcu_add_test of the root
  # CMakeLists.txt): Frequent always, Longer and Checkpoint only with
  # CALCU_TESTS_LONGER / _CHECKPOINT; every entry runs in this directory.
  calcu_add_test(CalcU1600CoreTests frequent COMMAND CalcU1600CoreTests)
  calcu_add_test(CalcU1600SessionTests frequent COMMAND CalcU1600SessionTests)
  calcu_add_test(CalcU1600AppSessionTests frequent COMMAND CalcU1600AppSessionTests)
  # Every preset of the repository is read by the library's reader through
  # `calcu check-preset`. The VS Code templates are checked filled in.
  set(CALCU1600_PRESET_GLOBS)
  foreach(ext pc1500 pc1500a pc1600)
    list(APPEND CALCU1600_PRESET_GLOBS examples/*/*.${ext} examples/*/*/*.${ext} tests/fixtures/presets/*.${ext}
         dev/hardware-checks/*.${ext} dev/presets/*.${ext} docs/developer/screenshots/presets/*.${ext}
         vscode/calcu1600-debug/presets/*.${ext})
  endforeach()
  calcu_add_preset_check(CalcU1600Presets PRODUCT c1600 FILES ${CALCU1600_PRESET_GLOBS})
  calcu_add_preset_check(CalcU1600Templates PRODUCT c1600 FILES vscode/calcu1600-debug/templates/*/debug.*
                         FILL tests/fill_vscode_templates.cmake)
  # No file above the core includes or names a machine class. CI runs it
  # with the core tests.
  calcu_add_test(CalcU1600CoreSeam frequent COMMAND sh tools/check_machine_includes.sh)
  # The library's source checks (calcu_product_source_checks): the ROM
  # lookup's allowed files are the host drive's own ROM
  # (core/Machine/SharpRoms.cpp) and the catalogue's ROM entries.
  calcu_product_source_checks(PREFIX CalcU1600 ROM_ALLOW core/Machine/SharpRoms.cpp core/Machine/SharpCatalogue.cpp)
  # Open With offers the three preset extensions on macOS, Windows and
  # Linux; on macOS .bas, .bbin, .bin as Alternate, no .rom.
  calcu_add_test(CalcU1600OpenWithTypes frequent COMMAND sh tools/check_open_with_types.sh)
  # The frontends take their colours from app/'s chrome (SharpChrome).
  calcu_add_test(CalcU1600ChromeColours frequent COMMAND sh ${CMAKE_SOURCE_DIR}/tools/check_chrome_colours.sh .)
  # The harness writes a PC-1600 trace,
  # tools/read_trace.py reads it unchanged (well under 2 s: Frequent).
  if(Python3_FOUND AND NOT WIN32)
    calcu_add_test(CalcU1600TraceReader frequent COMMAND sh tests/read_trace_check.sh
                   PROPERTIES ENVIRONMENT CALCU=$<TARGET_FILE:calcu>)
  endif()

  # The DAP server's cases, a program of their own so
  # that CalcU1600CoreTests' Frequent budget stays as it is (tests/dap/).
  file(GLOB DAP_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/dap/*.cpp)
  calcu1600_test_program(CalcU1600DapTests ${DAP_TEST_SOURCES})
  calcu_add_test(CalcU1600DapTests frequent COMMAND CalcU1600DapTests)
  # A DAP port outside 1024 to 65535 under the harness: `serve --dap 80`
  # ends with exit status 2.
  if(NOT WIN32)
    calcu_add_test(CalcU1600DapPortCheck frequent COMMAND sh tests/dap/dap_port_check.sh
                   PROPERTIES ENVIRONMENT CALCU=$<TARGET_FILE:calcu>)
  endif()
  # The scenarios of tools/dap_smoke.py against
  # `calcu --product c1600 serve --dap` (no window; ~17 s with its pauses by hand:
  # Longer). Where attach is built: the harness listens with attach's socket.
  if(Python3_FOUND AND NOT WIN32 AND CALCU_ATTACH)
    calcu_add_test(CalcU1600DapSmoke longer
                   COMMAND ${Python3_EXECUTABLE} tools/dap_smoke.py --port 32170 --harness $<TARGET_FILE:calcu>
                   PROPERTIES RUN_SERIAL TRUE)
  endif()

  # The help. CalcU1600HelpTests compares
  # help/settings-keys.txt with the declaration and help.yaml's names with
  # the catalogue (a program of its own, so that CalcU1600CoreTests' budget stays);
  # tools/help checks the help and builds Calc-U-1600-help.zip into the
  # tree (the test suite builds the asset). Python 3, standard library.
  calcu1600_test_program(CalcU1600HelpTests ${CMAKE_CURRENT_SOURCE_DIR}/tests/help/help_tests.cpp)
  calcu_add_test(CalcU1600HelpTests frequent COMMAND CalcU1600HelpTests)
  calcu_add_help_tests(CalcU1600 calc-u-1600)

  # The product commands under
  # `c1600` and the harness entry through SharpAppPart, each test on a
  # `calcu` run of its own; a program of its own, so that CalcU1600CoreTests' budget
  # stays (tests/harness/). About 10 s: Longer.
  file(GLOB HARNESS_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/harness/*.cpp)
  calcu1600_test_program(CalcU1600HarnessTests ${HARNESS_TEST_SOURCES})
  calcu_add_test(CalcU1600HarnessTests longer COMMAND CalcU1600HarnessTests)

  # The machine state, `state-save` / `state-restore` on
  # `calcu` runs (tests/machine_state_tests.cpp); a program of its own, so
  # that CalcU1600CoreTests' budget stays. Longer.
  calcu1600_test_program(CalcU1600MachineStateTests ${CMAKE_CURRENT_SOURCE_DIR}/tests/machine_state_tests.cpp)
  calcu_add_test(CalcU1600MachineStateTests longer COMMAND CalcU1600MachineStateTests)

  # The pytest driver on `calcu serve` (docs/testing/README.md): its own
  # tests, with pytest through uv (fetched once, so Longer). Where uv is.
  find_program(CALCU_UV uv)
  if(CALCU_UV)
    calcu_add_test(CalcU1600MachineDriver longer
                   COMMAND ${CMAKE_COMMAND} -E env CALCU=$<TARGET_FILE:calcu> PYTHONDONTWRITEBYTECODE=1
                           ${CALCU_UV} run --no-project --with pytest==8.3.5 --with pytest-xdist==3.6.1
                           pytest -q -n auto tools/machine)
    # The testing guide's examples (docs/testing/README.md) as a test project of their own.
    calcu_add_test(CalcU1600TestingExamples longer
                   COMMAND ${CMAKE_COMMAND} -E env CALCU=$<TARGET_FILE:calcu> PYTHONDONTWRITEBYTECODE=1
                           ${CALCU_UV} run --no-project --with pytest==8.3.5 --with pytest-xdist==3.6.1
                           pytest -q -n auto docs/testing/example)
  endif()

  # Longer: run locally before merging into main.
  # Against frontends/web/dist/ (frontends/web/build.sh first); the WASM
  # smoke builds in build/web itself (Emscripten).
  calcu_add_test(CalcU1600WebPlaywright longer COMMAND frontends/web/run-tests.sh)
  calcu_add_test(CalcU1600WasmSmoke longer COMMAND tools/build_wasm_check.sh)
  # The runners' Longer TESTs (TEST_IN(longer, …)): the examples against
  # their reference runs; full speed with a breakpoint set, printed.
  calcu_add_test(CalcU1600CoreTests.longer longer COMMAND CalcU1600CoreTests --group longer)
  calcu_add_test(CalcU1600SessionTests.longer longer COMMAND CalcU1600SessionTests --group longer)
  # A Web dist/ built from a
  # ROM folder without one file names it and starts no calculator.
  if(NOT WIN32)
    calcu_add_test(CalcU1600RomMissingBuild longer COMMAND sh tests/rom_missing_build.sh PROPERTIES RUN_SERIAL TRUE)
    # A release Web dist/ (build.sh --release)
    # has no c1600.upload and no c1600.display-text; CI checks the dist/ it
    # zips the same way.
    calcu_add_test(CalcU1600WebNoTestAids longer COMMAND sh tests/web_no_test_aids.sh PROPERTIES RUN_SERIAL TRUE)
  endif()
  if(TARGET CalcU1600Qt)
    # The screenshot scenarios (one per help chapter) run as checks: every
    # shot is taken, into the build tree, not help/images/.
    # The GUI check scenarios (tests/gui/) run the same way, under their
    # names.
    file(GLOB CALCU1600_SCENARIOS ${CMAKE_CURRENT_SOURCE_DIR}/docs/developer/screenshots/guide/*.shots.yaml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/tests/gui/*.shots.yaml)
    foreach(scenario ${CALCU1600_SCENARIOS})
      get_filename_component(name ${scenario} NAME_WE)
      # The SHIFT-tap scenario sends host key events to the window:
      # offscreen, no window shown.
      set(shot_environment)
      if(name STREQUAL "shift-tap-types-second-legend")
        set(shot_environment ENVIRONMENT QT_QPA_PLATFORM=offscreen)
      endif()
      # The Debugger section with the server listening.
      set(shot_arguments)
      if(name STREQUAL "debugger-settings-disconnect")
        set(shot_arguments --dap 32177)
      endif()
      calcu_add_test(CalcU1600Shots.${name} longer
                     COMMAND CalcU1600Qt --shots ${scenario} --shots-out ${CMAKE_CURRENT_BINARY_DIR}/shots-check/${name} ${shot_arguments}
                     PROPERTIES RUN_SERIAL TRUE ${shot_environment})
    endforeach()
    # The scenarios of tools/dap_smoke.py against the Qt app (with its
    # vscode_session).
    if(Python3_FOUND)
      calcu_add_test(CalcU1600DapSmokeQt longer
                     COMMAND ${Python3_EXECUTABLE} tools/dap_smoke.py --port 32169 --app $<TARGET_FILE:CalcU1600Qt>
                     PROPERTIES RUN_SERIAL TRUE)
    endif()
  endif()

  # Checkpoint: run before a release.
  # The runner's Checkpoint TESTs: ten minutes per model on the real clock
  # (PC-1500A, then PC-1600).
  calcu_add_test(CalcU1600SessionTests.checkpoint checkpoint COMMAND CalcU1600SessionTests --group checkpoint
                 PROPERTIES TIMEOUT 1500)
  if(TARGET CalcU1600Qt)
    # A PC-1600 keeps one minute in the Qt6 frontend (the --pacing-probe
    # test hook of non-release builds), offscreen with a home of its own: the
    # user's settings (a startup model or preset) would change what runs.
    set(PROBE_HOME ${CMAKE_CURRENT_BINARY_DIR}/pacing-probe-home)
    file(MAKE_DIRECTORY ${PROBE_HOME})
    calcu_add_test(CalcU1600QtPacingProbe checkpoint COMMAND CalcU1600Qt --pacing-probe 60
                   PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
    if(TEST CalcU1600QtPacingProbe)   # registered only with CALCU_TESTS_CHECKPOINT
      set_tests_properties(CalcU1600QtPacingProbe PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${PROBE_HOME}")
    endif()
    # The help's pictures into help/images/, one scenario per
    # help chapter (tools/make_screenshots.sh builds the app in build/app-1600).
    calcu_add_test(CalcU1600HelpPictures checkpoint COMMAND tools/make_screenshots.sh PROPERTIES RUN_SERIAL TRUE)
  endif()
endif()

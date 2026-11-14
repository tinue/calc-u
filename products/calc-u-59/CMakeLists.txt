cmake_minimum_required(VERSION 3.21)

# Calc-U-59 in the repository's root CMake project (ADR 0003; option
# CALCU_PRODUCT_59): the emulation core, the app/ C++ layer the frontends
# use, the C++ test suite (CTest, with CALCU_TESTS), the headless harness
# library, and, with CALCU_FRONTEND_QT and when Qt6 is found, the Qt6
# desktop app via add_subdirectory(frontends/qt6). Under Emscripten only
# the core, the app layer and the Web engine (calcu59_web).
#
# The Swift app is built by Calc-U-59.xcodeproj, which compiles the same
# core/ and app/ sources (synchronized folders). Every core/ or app/
# change must pass both builds — see CLAUDE.md § "Frontend Parity".

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "products/calc-u-59 is part of the root CMake project (ADR 0003): "
                      "configure the repository root, e.g. cmake -S ../.. -B ../../build/all")
endif()

project(Calc-U-59 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
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
add_library(calcu59_core STATIC
  ${CORE_ROOT}/TI59Machine.cpp
  ${CORE_ROOT}/TMC0501.cpp
  ${CORE_ROOT}/ROM.cpp
  ${CORE_ROOT}/RAM.cpp
)
target_include_directories(calcu59_core PUBLIC ${CORE_ROOT})
target_compile_options(calcu59_core PRIVATE ${CALCU59_WARNINGS})
set_target_properties(calcu59_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
find_package(Threads REQUIRED)
target_link_libraries(calcu59_core PUBLIC Threads::Threads calcu_library)

file(GLOB SHARED_SOURCES CONFIGURE_DEPENDS ${SHARED_ROOT}/*.cpp)
add_library(calcu59_shared STATIC ${SHARED_SOURCES})
target_include_directories(calcu59_shared PUBLIC ${SHARED_ROOT})
target_link_libraries(calcu59_shared PUBLIC calcu59_core)
target_compile_options(calcu59_shared PRIVATE ${CALCU59_WARNINGS})
# Both static libraries end up in the ti59headless shared object: on Linux
# that needs -fPIC (macOS always builds position-independent code).
set_target_properties(calcu59_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# Web engine (emcmake; frontends/web/build.sh copies it into
# frontends/web/dist/wasm/, reference/WebClient.md): core/ + app/ behind
# frontends/web/bindings.cpp. Single-threaded (no -pthread): a static host
# can't be relied on to send the headers SharedArrayBuffer needs, so
# SessionController runs in Threading::Cooperative. -fexceptions: app/
# reports ROM and file errors with exceptions (Emscripten's JS-based
# model; Calc-U-1600's engine uses native wasm exceptions, which its
# libsharpdx needs). -UNDEBUG: the core's asserts stay on, as in the em++
# build this replaces (-O3 without -DNDEBUG).
# -----------------------------------------------------------------------------
if(EMSCRIPTEN)
  foreach(target calcu59_core calcu59_shared)
    target_compile_options(${target} PRIVATE -fexceptions -UNDEBUG)
  endforeach()
  add_executable(calcu59_web ${CMAKE_CURRENT_SOURCE_DIR}/frontends/web/bindings.cpp)
  target_link_libraries(calcu59_web PRIVATE calcu59_shared)
  target_compile_options(calcu59_web PRIVATE -fexceptions -UNDEBUG)
  set_target_properties(calcu59_web PROPERTIES OUTPUT_NAME ti59-core)
  target_link_options(calcu59_web PRIVATE -fexceptions -lembind -sMODULARIZE=1
                      -sEXPORT_NAME=createTI59CoreModule -sENVIRONMENT=web,worker
                      -sALLOW_MEMORY_GROWTH=1 -sNO_EXIT_RUNTIME=1)
  return()
endif()

# -----------------------------------------------------------------------------
# Headless harness (tools/headless/ti59.py loads it via ctypes; build.sh does
# the same with one clang++ call)
# -----------------------------------------------------------------------------
add_library(ti59headless SHARED ${CMAKE_CURRENT_SOURCE_DIR}/tools/headless/capi.cpp)
target_link_libraries(ti59headless PRIVATE calcu59_shared)

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
  target_link_libraries(CalcU59Tests PRIVATE calcu59_shared)
  target_compile_options(CalcU59Tests PRIVATE ${CALCU59_WARNINGS})
  if(NOT MSVC)
    target_compile_options(CalcU59Tests PRIVATE $<$<CONFIG:Debug>:-O1>)
  endif()

  enable_testing()
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)  # scratch files written by the tests
  add_test(NAME CalcU59Tests COMMAND CalcU59Tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  # tools/read_trace.py must read the trace CalcU59Tests wrote with Shared/TraceWriter.
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND)
    add_test(NAME CalcU59TraceReader
             COMMAND ${Python3_EXECUTABLE} tools/read_trace.py --json build/trace-test.bin
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(CalcU59TraceReader PROPERTIES DEPENDS CalcU59Tests)
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

cmake_minimum_required(VERSION 3.21)

# The one CMake project of the repository: the library, the
# cores, app layers, tests, tools and Qt6 frontends of every product. Local
# builds, IDEs, the harness and CI configure this file; the product
# directories are its subdirectories and do not configure on their own.
#
#   cmake -S . -B build/all -G Ninja                         # everything, Qt6 when found
#   cmake -S . -B build/tests -G Ninja -DCALCU_FRONTEND_QT=OFF  # cores and tests, no Qt
#   ctest --test-dir build/tests/products/calc-u-59          # one product's tests
#   ctest --test-dir build/tests -L frequent                 # one test group
#   emcmake cmake -S . -B build/web -G Ninja -DCMAKE_BUILD_TYPE=Release  # the Web engines
#
# Options:
#   CALCU_PRODUCT_59    Calc-U-59 (products/calc-u-59/)
#   CALCU_PRODUCT_1600  Calc-U-1600 (products/calc-u-1600/)
#   CALCU_FRONTEND_QT   the Qt6 desktop apps; without it nothing needs Qt
#   CALCU_TESTS         the products' test suites; the release packaging in
#                       CI turns them off (build/release, the Qt6 apps only)
#   CALCU_TESTS_LONGER      also the Longer group's tests: Playwright,
#                           the WASM smoke, XCUITest, screenshot scenarios, DAP
#   CALCU_TESTS_CHECKPOINT  also the Checkpoint group's tests: the screenshot
#                           sets and the ten-minute pacing runs
#   CALCU_TESTS_DEBUG   the cores and test programs at -O0 in a Debug tree
#                       (else -O1: calcu_debug_o1 below), for stepping through
#   CALCU_SANITIZE      sanitizers for every target (below), the tree
#                       build/asan of docs/test-groups.md
#
# Test groups: every CTest entry carries the label frequent,
# longer or checkpoint. Without the two options a tree has only the
# Frequent group, so a plain ctest is the Frequent run. docs/test-groups.md
# lists every test and its group.
#
# Under the Emscripten toolchain (emcmake) each product defines only its
# Web engine (calcu59_web, calcu1600_web) and Calc-U-1600's wasm_smoke check;
# frontends/web/build.sh of each product builds it in build/web and puts
# dist/ together. The Swift frontend's XCFramework:
# products/calc-u-59/frontends/swift/build_xcframework.sh configures this
# file for macOS, iOS and the iOS simulator in build/xcframework-59; the
# Xcode project runs it before every build.

# One minimum for the whole tree, before project(), so both products' Mac
# apps (the Qt6 builds) need the same macOS: 15.8, which Calc-U-1600's
# vendored libsharpdx is built for.
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.8" CACHE STRING "Minimum macOS version")

if(APPLE)
  # The Qt6 component set and Calc-U-1600's Qt6 app have Objective-C++ sources.
  project(Calc-U LANGUAGES CXX OBJCXX)
else()
  project(Calc-U LANGUAGES CXX)
endif()

option(CALCU_PRODUCT_59 "Build Calc-U-59" ON)
option(CALCU_PRODUCT_1600 "Build Calc-U-1600" ON)
option(CALCU_FRONTEND_QT "Build the Qt6 desktop apps" ON)
option(CALCU_TESTS "Build the products' test suites" ON)
option(CALCU_TESTS_LONGER "Also register the Longer group's tests" OFF)
option(CALCU_TESTS_CHECKPOINT "Also register the Checkpoint group's tests" OFF)

option(CALCU_TESTS_DEBUG "Build the cores and the test programs at -O0 in Debug, for stepping through" OFF)

# CALCU_SANITIZE: the sanitizers every target is built with, as -fsanitize
# takes them (address,undefined; thread alone), and the standard library's
# own bounds checks (libc++ hardening, libstdc++ assertions) with them.
# A finding stops the program, so the test that ran into it fails. Not for
# MSVC or Emscripten.
set(CALCU_SANITIZE "" CACHE STRING "Sanitizers for every target (address,undefined or thread); empty for none")
if(CALCU_SANITIZE)
  if(MSVC OR EMSCRIPTEN)
    message(FATAL_ERROR "CALCU_SANITIZE needs Clang or GCC on macOS or Linux")
  endif()
  add_compile_options(-fsanitize=${CALCU_SANITIZE} -fno-sanitize-recover=all -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=${CALCU_SANITIZE})
  add_compile_definitions(_GLIBCXX_ASSERTIONS _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

# calcu_add_test(<name> <group> COMMAND <command…> [WORKING_DIRECTORY <dir>]
#                [PROPERTIES <property> <value> …])
# A CTest entry in its test group: <group> is frequent, longer
# or checkpoint, and becomes its label; a Longer or Checkpoint entry is
# registered only with CALCU_TESTS_LONGER / CALCU_TESTS_CHECKPOINT. It runs
# in the calling directory's source folder (the product directory) unless
# WORKING_DIRECTORY says otherwise.
function(calcu_add_test name group)
  cmake_parse_arguments(T "" "WORKING_DIRECTORY" "COMMAND;PROPERTIES" ${ARGN})
  if(NOT group MATCHES "^(frequent|longer|checkpoint)$")
    message(FATAL_ERROR "calcu_add_test(${name}): group ${group} is not frequent, longer or checkpoint")
  endif()
  if((group STREQUAL "longer" AND NOT CALCU_TESTS_LONGER) OR (group STREQUAL "checkpoint" AND NOT CALCU_TESTS_CHECKPOINT))
    return()
  endif()
  if(NOT DEFINED T_WORKING_DIRECTORY)
    set(T_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  add_test(NAME ${name} COMMAND ${T_COMMAND} WORKING_DIRECTORY ${T_WORKING_DIRECTORY})
  set_tests_properties(${name} PROPERTIES LABELS ${group} ${T_PROPERTIES})
endfunction()

# calcu_build_id_header(<name> <product directory> <target…>): the calling
# directory's generated/BuildId.h with APP_BUILD_ID (tools/build-id.cmake),
# made again at each build of the targets, which include generated/.
function(calcu_build_id_header name product)
  set(header ${CMAKE_CURRENT_BINARY_DIR}/generated/BuildId.h)
  add_custom_target(${name}
    COMMAND ${CMAKE_COMMAND} -DPRODUCT=${product} -DHEADER=${header} -P ${CMAKE_SOURCE_DIR}/tools/build-id.cmake
    BYPRODUCTS ${header}
    COMMENT "Build ID of ${product}"
    VERBATIM)
  foreach(target ${ARGN})
    add_dependencies(${target} ${name})
  endforeach()
endfunction()

# calcu_add_help_tests(<prefix> <product>): <prefix>HelpCheck and
# <prefix>HelpBuild, Frequent: tools/help checks the product's help and
# builds its asset into the calling directory's tree (help/), so the test
# suite builds the asset too. Registered when Python 3 is found.
function(calcu_add_help_tests prefix product)
  if(Python3_FOUND)
    set(help ${Python3_EXECUTABLE} -I -B ${CMAKE_SOURCE_DIR}/tools/help)
    calcu_add_test(${prefix}HelpCheck frequent COMMAND ${help} check ${product} --quiet-warnings)
    calcu_add_test(${prefix}HelpBuild frequent
                   COMMAND ${help} build ${product} --quiet-warnings --out ${CMAKE_CURRENT_BINARY_DIR}/help)
  endif()
endfunction()

# The warnings of every target the repository compiles (linked PRIVATE):
# -Wall -Wextra, or MSVC's /W4 with /utf-8 (the sources are UTF-8 without a
# BOM; MSVC would read them in the ANSI code page).
add_library(calcu_warnings INTERFACE)
if(MSVC)
  target_compile_options(calcu_warnings INTERFACE /W4 /utf-8)
else()
  target_compile_options(calcu_warnings INTERFACE -Wall -Wextra)
endif()

# calcu_debug_o1(<target>…): a core or a test program at -O1 in Debug, so
# that the ROM-driven tests stay usable (about three times faster than -O0);
# CALCU_TESTS_DEBUG keeps -O0 for stepping through a failing test.
function(calcu_debug_o1)
  if(NOT MSVC AND NOT CALCU_TESTS_DEBUG)
    foreach(t ${ARGN})
      target_compile_options(${t} PRIVATE $<$<CONFIG:Debug>:-O1>)
    endforeach()
  endif()
endfunction()

if(CALCU_FRONTEND_QT AND APPLE AND NOT DEFINED CMAKE_PREFIX_PATH)
  # Homebrew's Qt, for both products' frontends.
  foreach(prefix /opt/homebrew/opt/qt /usr/local/opt/qt)
    if(EXISTS ${prefix})
      list(APPEND CMAKE_PREFIX_PATH ${prefix})
      break()
    endif()
  endforeach()
endif()

# Python 3 for the tests that run a script (the help tool, the trace
# readers, the harness's Python helper); a test without it is not registered.
if(CALCU_TESTS)
  find_package(Python3 COMPONENTS Interpreter QUIET)
endif()

add_subdirectory(library)

if(CALCU_PRODUCT_59)
  add_subdirectory(products/calc-u-59)
endif()
if(CALCU_PRODUCT_1600)
  add_subdirectory(products/calc-u-1600)
endif()

# The harness `calcu`: one program for every product
# built. The library may not depend on product code, so each
# product registers its factory with
#
#   calcu_harness_add_product(<its app target> <namespace::factory>)
#
# and a call deferred to the end of the top-level directory, after every
# product directory has been read, writes the table products() from
# Products.cpp.in and links each product target into `calcu`. No static
# initialiser: one in a static library is dropped by the linker.
# `calcu_test` is the same program with the product made for the tests only
# (CALCU_TESTS), so the CLI is tested without product code.
# Not under Emscripten and not for iOS: the harness is a desktop program.

set_property(GLOBAL PROPERTY CALCU_HARNESS_PRODUCTS "")

function(calcu_harness_add_product target factory)
  set_property(GLOBAL APPEND PROPERTY CALCU_HARNESS_PRODUCTS "${target}|${factory}")
endfunction()

# Writes <out> from Products.cpp.in for the "<target>|<ns::factory>" entries.
function(calcu_harness_write_products out)
  set(decls "")
  set(calls "")
  foreach(entry ${ARGN})
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 1 factory)
    string(FIND "${factory}" "::" sep REVERSE)
    if(sep LESS 0)
      set(decls "${decls}std::unique_ptr<calcu::harness::HarnessProduct> ${factory}();\n")
    else()
      string(SUBSTRING "${factory}" 0 ${sep} ns)
      math(EXPR start "${sep} + 2")
      string(SUBSTRING "${factory}" ${start} -1 fn)
      set(decls "${decls}namespace ${ns} { std::unique_ptr<calcu::harness::HarnessProduct> ${fn}(); }\n")
    endif()
    set(calls "${calls}    out.push_back(${factory}());\n")
  endforeach()
  set(CALCU_HARNESS_DECLARATIONS "${decls}")
  set(CALCU_HARNESS_CALLS "${calls}")
  configure_file(${CALCU_HARNESS_CLI_DIR}/Products.cpp.in ${out} @ONLY)
endfunction()

if(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
  return()
endif()

set(CALCU_HARNESS_CLI_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")
set(CALCU_HARNESS_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "")

function(calcu_harness_cli_target name)
  add_executable(${name} ${CALCU_HARNESS_CLI_DIR}/Main.cpp ${CALCU_HARNESS_CLI_DIR}/Cli.cpp ${ARGN})
  target_include_directories(${name} PRIVATE ${CALCU_HARNESS_CLI_DIR})
  target_link_libraries(${name} PRIVATE calcu_library calcu_warnings)
endfunction()

calcu_harness_write_products(${CMAKE_CURRENT_BINARY_DIR}/Products.cpp)   # none until the deferred call
calcu_harness_cli_target(calcu ${CMAKE_CURRENT_BINARY_DIR}/Products.cpp)

function(calcu_harness_finish)
  get_property(entries GLOBAL PROPERTY CALCU_HARNESS_PRODUCTS)
  calcu_harness_write_products(${CALCU_HARNESS_BIN_DIR}/Products.cpp ${entries})
  foreach(entry ${entries})
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 target)
    target_link_libraries(calcu PRIVATE ${target})
  endforeach()
endfunction()
cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL calcu_harness_finish)

if(CALCU_TESTS)
  set(lib ${PROJECT_SOURCE_DIR}/library)
  calcu_harness_write_products(${CMAKE_CURRENT_BINARY_DIR}/TestProducts.cpp
                               "test|calcu::harness::test::makeTestProduct")
  calcu_harness_cli_target(calcu_test ${CMAKE_CURRENT_BINARY_DIR}/TestProducts.cpp
                           ${lib}/harness/tests/TestProduct.cpp)
  target_link_libraries(calcu_test PRIVATE calcu_test_machines)

  # The program in a process of its own (argv, stdin, exit status),
  # run with cmake -P (Frequent; docs/test-groups.md); one entry per case of
  # cli_cases.cmake, read from its `CASE STREQUAL "<case>"` branches.
  file(STRINGS ${lib}/harness/tests/cli/cli_cases.cmake case_lines REGEX "CASE STREQUAL \"[a-z0-9_]+\"")
  set(cases "")
  foreach(line ${case_lines})
    string(REGEX REPLACE ".*CASE STREQUAL \"([a-z0-9_]+)\".*" "\\1" c "${line}")
    list(APPEND cases ${c})
  endforeach()
  foreach(case ${cases})
    calcu_add_test(calcu_cli.${case} frequent
                   COMMAND ${CMAKE_COMMAND} -DCALCU=$<TARGET_FILE:calcu_test> -DCASE=${case}
                           -DWORK=${CMAKE_CURRENT_BINARY_DIR}/work-${case}
                           -P ${lib}/harness/tests/cli/cli_cases.cmake
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endforeach()

  # The Python helper for `calcu serve`, with uv when there is one,
  # else with Python 3 (Frequent).
  find_program(CALCU_UV uv)
  if(CALCU_UV)
    set(py ${CALCU_UV} run --no-project python -I -B)
  elseif(Python3_FOUND)
    set(py ${Python3_EXECUTABLE} -I -B)
  endif()
  if(DEFINED py)
    calcu_add_test(calcu_serve_python frequent
                   COMMAND ${py} ${lib}/harness/python/tests/test_calcu_serve.py $<TARGET_FILE:calcu_test>
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()
  # The run report of the products' pytest plugins, with pytest through uv
  # (fetched once, so Longer). Where uv is.
  if(CALCU_UV)
    calcu_add_test(calcu_report_python longer
                   COMMAND ${CMAKE_COMMAND} -E env CALCU=$<TARGET_FILE:calcu_test> PYTHONDONTWRITEBYTECODE=1
                           ${CALCU_UV} run --no-project --with pytest==8.3.5 pytest -q
                   WORKING_DIRECTORY ${lib}/harness/python)
  endif()
endif()

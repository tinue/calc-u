# The harness `calcu` (LIB-11 design §2, §3.7): one program for every product
# built. The library may not depend on product code (LIB-1 AC-24), so each
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
  target_link_libraries(${name} PRIVATE calcu_library)
  if(MSVC)
    target_compile_options(${name} PRIVATE /W4 /utf-8)
  else()
    target_compile_options(${name} PRIVATE -Wall -Wextra)
  endif()
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
                           ${lib}/harness/tests/TestProduct.cpp ${lib}/machine/tests/TestMachines.cpp)
  target_include_directories(calcu_test PRIVATE ${lib}/machine/tests ${lib}/roms/tests ${lib}/harness/tests ${lib}/testing)

  # T13: the program in a process of its own (argv, stdin, exit status),
  # run with cmake -P (Frequent; docs/test-groups.md).
  foreach(case
      ac1_product_by_name_or_short_form_any_case
      ac2_one_shot_runs_in_order_and_exits_zero
      ac2_first_refusal_exits_nonzero_with_reason
      ac3_serve_one_reply_per_line_until_quit
      ec3_serve_ends_at_end_of_input
      ec4_unknown_product_lists_products)
    add_test(NAME calcu_cli.${case}
             COMMAND ${CMAKE_COMMAND} -DCALCU=$<TARGET_FILE:calcu_test> -DCASE=${case}
                     -DWORK=${CMAKE_CURRENT_BINARY_DIR}/work-${case}
                     -P ${lib}/harness/tests/cli/cli_cases.cmake)
    set_tests_properties(calcu_cli.${case} PROPERTIES LABELS frequent)
  endforeach()

  # T15: the Python helper for `calcu serve` (D-2), with uv when there is one,
  # else with Python 3 (Frequent).
  find_program(CALCU_UV uv)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(CALCU_UV)
    set(py ${CALCU_UV} run --no-project python -I -B)
  elseif(Python3_FOUND)
    set(py ${Python3_EXECUTABLE} -I -B)
  endif()
  if(DEFINED py)
    add_test(NAME calcu_serve_python
             COMMAND ${py} ${lib}/harness/python/tests/test_calcu_serve.py $<TARGET_FILE:calcu_test>)
    set_tests_properties(calcu_serve_python PROPERTIES LABELS frequent)
  endif()
endif()

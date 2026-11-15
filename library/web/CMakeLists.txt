# LIB-15 Web frontend shell (design §2): JavaScript in src/, app/, build/;
# the C++ glue in glue/ (namespace calcu::web); the tests in tests/.
#
# Under Emscripten: calcu_web_glue, an INTERFACE target whose sources each
# product's Web engine compiles with its own flags (LIB-1 design §2a, the one
# exception to the one library target; LIB-15 design §3.6), and the product
# made for the tests, calcu_web_test_product (tests/product/build.sh puts it
# into build/web/library/web/test-dist/).
# Natively: the CTest entries LibraryWebNodeTests (Frequent, node --test) and,
# with CALCU_TESTS_LONGER, LibraryWebPlaywright (Longer), each beside
# calcu_library_tests because they run Node and browsers, not C++.
if(EMSCRIPTEN)
  add_library(calcu_web_glue INTERFACE)
  target_sources(calcu_web_glue INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/glue/MessageCodec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/glue/WorkerHost.cpp)
  target_link_libraries(calcu_web_glue INTERFACE calcu_library)

  add_executable(calcu_web_test_product
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/product/TestProduct.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../machine/tests/TestMachines.cpp)
  target_link_libraries(calcu_web_test_product PRIVATE calcu_web_glue)
  target_compile_options(calcu_web_test_product PRIVATE -fwasm-exceptions -Wall -Wextra)
  set_target_properties(calcu_web_test_product PROPERTIES OUTPUT_NAME calcutest-core)
  target_link_options(calcu_web_test_product PRIVATE -fwasm-exceptions -lembind -lidbfs.js -sMODULARIZE=1
                      -sEXPORT_NAME=createCalcUTestModule -sENVIRONMENT=worker -sALLOW_MEMORY_GROWTH=1
                      -sEXPORTED_RUNTIME_METHODS=FS,IDBFS)
  return()
endif()

if(CALCU_TESTS)
  find_program(CALCU_NODE node)
  if(CALCU_NODE)
    add_test(NAME LibraryWebNodeTests
             COMMAND ${CALCU_NODE} --test ${CMAKE_CURRENT_SOURCE_DIR}/tests/build.test.js)
    set_tests_properties(LibraryWebNodeTests PROPERTIES LABELS frequent)
  endif()
  if(CALCU_TESTS_LONGER AND NOT WIN32)
    add_test(NAME LibraryWebPlaywright COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-tests.sh --reporter=line)
    set_tests_properties(LibraryWebPlaywright PROPERTIES LABELS longer TIMEOUT 900)
  endif()
endif()

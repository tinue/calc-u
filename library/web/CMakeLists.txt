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

  # calcu_web_engine(<target> SOURCES <file>… OUTPUT <name> EXPORT <factory>
  #                  ENVIRONMENT <env> [LINK <option>…]): a Web engine for the
  # library's worker: the sources with the glue, native wasm exceptions (one
  # build/web tree holds every engine; LIB-1 design §2a, D-6), embind, IDBFS
  # for the persistent file system (mounted by the worker), a modularised
  # <name>.js whose factory is <factory>, a growing memory. No -pthread: a
  # static host can't be relied on to send the headers SharedArrayBuffer
  # needs. LINK adds the engine's own options.
  function(calcu_web_engine target)
    cmake_parse_arguments(W "" "OUTPUT;EXPORT;ENVIRONMENT" "SOURCES;LINK" ${ARGN})
    add_executable(${target} ${W_SOURCES})
    target_link_libraries(${target} PRIVATE calcu_web_glue)
    target_compile_options(${target} PRIVATE -fwasm-exceptions)
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${W_OUTPUT})
    target_link_options(${target} PRIVATE -fwasm-exceptions -lembind -lidbfs.js -sMODULARIZE=1
                        -sEXPORT_NAME=${W_EXPORT} -sENVIRONMENT=${W_ENVIRONMENT} -sALLOW_MEMORY_GROWTH=1
                        ${W_LINK})
  endfunction()

  calcu_web_engine(calcu_web_test_product
                   SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/product/TestProduct.cpp
                           ${CMAKE_CURRENT_SOURCE_DIR}/../machine/tests/TestMachines.cpp
                           ${CMAKE_CURRENT_SOURCE_DIR}/../machine/tests/TestCpu.cpp
                           ${CMAKE_CURRENT_SOURCE_DIR}/../media/tests/TestKinds.cpp
                   OUTPUT calcutest-core EXPORT createCalcUTestModule ENVIRONMENT worker
                   LINK -sEXPORTED_RUNTIME_METHODS=FS,IDBFS)
  target_link_libraries(calcu_web_test_product PRIVATE calcu_warnings)
  # TestKinds.cpp includes "TestMachines.hpp" as the library's test programs do.
  target_include_directories(calcu_web_test_product PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../machine/tests)
  return()
endif()

if(CALCU_TESTS)
  # A tree configured where node was (the local Linux CI's web image) and
  # rebuilt where it is not keeps a cached path that no longer exists.
  if(CALCU_NODE AND NOT EXISTS "${CALCU_NODE}")
    unset(CALCU_NODE CACHE)
  endif()
  find_program(CALCU_NODE node)
  if(CALCU_NODE)
    calcu_add_test(LibraryWebNodeTests frequent
                   COMMAND ${CALCU_NODE} --test ${CMAKE_CURRENT_SOURCE_DIR}/tests/build.test.js
                           ${CMAKE_CURRENT_SOURCE_DIR}/tests/shell_model.test.js    # LIB-14: the shell's models
                           ${CMAKE_CURRENT_SOURCE_DIR}/tests/host_keys.test.js      # LIB-5: host keys
                           ${CMAKE_CURRENT_SOURCE_DIR}/tests/embed_props.test.js    # LIB-15 AC-29: the shared props
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()
  if(NOT WIN32)
    calcu_add_test(LibraryWebPlaywright longer COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-tests.sh --reporter=line
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} PROPERTIES TIMEOUT 900)
  endif()
endif()

# The Qt6 component set (LIB-8 design §2, D-1; LIB-14 design §2.2, D-16):
# the library's widgets for the Qt6 frontends, in namespace calcu::qt6. A
# static target of its own, calcu_qt6, because calcu_library stays free of Qt
# for the Web and Swift builds (the fourth exception of LIB-1 §2a). Built only
# with CALCU_FRONTEND_QT and when Qt6 is found, never under Emscripten or for
# iOS (library/CMakeLists.txt). LIB-8 (M-39) creates it with the paper panel;
# LIB-10 (M-40), LIB-7 (M-41) and LIB-14's shell (M-43) add their files here.
#
# Its tests: one program, calcu_qt6_tests, every library/qt6/tests/*_tests.cpp
# on the repository's one runner (ADR 0038) with a QApplication; Frequent, on
# the offscreen platform.
find_package(Qt6 QUIET COMPONENTS Widgets)
if(NOT Qt6_FOUND)
  message(STATUS "Qt6 not found: no library Qt6 component set (calcu_qt6)")
  return()
endif()

add_library(calcu_qt6 STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/PaperPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PaperPainter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ClipboardService.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/RunControlButtons.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MediaPicker.cpp          # LIB-7 (M-41)
  ${CMAKE_CURRENT_SOURCE_DIR}/NameAndSaveDialog.cpp)
set_target_properties(calcu_qt6 PROPERTIES AUTOMOC ON)
target_link_libraries(calcu_qt6 PUBLIC calcu_library Qt6::Widgets PRIVATE calcu_warnings)
target_include_directories(calcu_qt6 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(calcu_qt6 PUBLIC cxx_std_20)

if(CALCU_TESTS)
  file(GLOB CALCU_QT6_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_tests.cpp)
  add_executable(calcu_qt6_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/QtTestMain.cpp ${CALCU_QT6_TEST_SOURCES})
  target_link_libraries(calcu_qt6_tests PRIVATE calcu_qt6 calcu_test_machines calcu_warnings)
  target_include_directories(calcu_qt6_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../testing)
  calcu_add_test(calcu_qt6_tests frequent COMMAND calcu_qt6_tests
                 PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
endif()

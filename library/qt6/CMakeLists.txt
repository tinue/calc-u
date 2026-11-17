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
# the offscreen platform. LIB-14 adds calcu_qt6_scenarios (the scenarios of
# its AC-25 on the test window) beside it.
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
# LIB-14 (M-43): the shell.
target_sources(calcu_qt6 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/SessionHost.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Shell.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellMenus.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellControlBar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellSettingsDialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellLoading.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellNotices.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellAbout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PanelArea.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/FaceplateInput.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/HostKeys.cpp          # LIB-5 AC-7: host keys to host-key-down
  ${CMAKE_CURRENT_SOURCE_DIR}/RomsMissingView.cpp)
# LIB-14 Part E (M-57): the in-process line client of harness attach, for
# the products' attach clients (Calc-U-1600's screenshot scenarios, both
# pacing probes); only with CALCU_ATTACH, so only those trees need Qt Network.
if(CALCU_ATTACH)
  find_package(Qt6 REQUIRED COMPONENTS Network)
  target_sources(calcu_qt6 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/AttachLineClient.cpp)
  target_link_libraries(calcu_qt6 PUBLIC Qt6::Network)
endif()
# LIB-8 D-17 (M-45): on macOS the picture of `clipboard-image` goes through
# Cocoa, which keeps its physical size (Qt's clipboard loses it).
if(APPLE)
  target_sources(calcu_qt6 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/MacClipboard.mm)
  target_link_libraries(calcu_qt6 PUBLIC "-framework Cocoa")
endif()
set_target_properties(calcu_qt6 PROPERTIES AUTOMOC ON)
target_link_libraries(calcu_qt6 PUBLIC calcu_library Qt6::Widgets PRIVATE calcu_warnings)
target_include_directories(calcu_qt6 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(calcu_qt6 PUBLIC cxx_std_20)
# Qt before 6.5 (Ubuntu 24.04's 6.4.2): its moc cannot parse `concept`, and
# with the target's own -std=gnu++20 AutoMoc's compiler predefines make it
# read libstdc++ 13's <concepts> (CI, M-45). Without the predefines moc skips
# those sections, as it does for the product apps, whose predefines are
# taken without -std.
function(calcu_qt6_moc_compat target)
  if(Qt6_VERSION VERSION_LESS 6.5)
    set_target_properties(${target} PROPERTIES AUTOMOC_COMPILER_PREDEFINES OFF)
  endif()
endfunction()
calcu_qt6_moc_compat(calcu_qt6)

if(CALCU_TESTS)
  # LIB-14: QTest's key and mouse clicks for the shortcut cases (AC-4); a Qt
  # without the Test module skips them.
  find_package(Qt6 QUIET COMPONENTS Test)
  # LIB-14's test window over TestShellPart (library/shell/tests/) and, with
  # CALCU_ATTACH, the harness's test product for the attach host (L8).
  set(CALCU_QT6_TEST_SUPPORT ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestWindow.cpp)
  if(CALCU_ATTACH)
    list(APPEND CALCU_QT6_TEST_SUPPORT ${CMAKE_CURRENT_SOURCE_DIR}/../harness/tests/TestProduct.cpp)
  endif()
  function(calcu_qt6_test_program name)
    add_executable(${name} ${CMAKE_CURRENT_SOURCE_DIR}/tests/QtTestMain.cpp ${CALCU_QT6_TEST_SUPPORT} ${ARGN})
    target_link_libraries(${name} PRIVATE calcu_qt6 calcu_test_machines calcu_warnings)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../testing
                                               ${CMAKE_CURRENT_SOURCE_DIR}/../shell/tests ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    set_target_properties(${name} PROPERTIES AUTOMOC ON)
    calcu_qt6_moc_compat(${name})
    if(TARGET Qt6::Test)
      target_link_libraries(${name} PRIVATE Qt6::Test)
      target_compile_definitions(${name} PRIVATE CALCU_QT_TEST=1)
    endif()
  endfunction()

  file(GLOB CALCU_QT6_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_tests.cpp)
  if(NOT CALCU_ATTACH)
    list(REMOVE_ITEM CALCU_QT6_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/attach_host_tests.cpp)
  endif()
  calcu_qt6_test_program(calcu_qt6_tests ${CALCU_QT6_TEST_SOURCES})
  calcu_add_test(calcu_qt6_tests frequent COMMAND calcu_qt6_tests
                 PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
  # LIB-14 L10: the scenarios of AC-25 under their IDs (ADR 0032).
  calcu_qt6_test_program(calcu_qt6_scenarios ${CMAKE_CURRENT_SOURCE_DIR}/tests/shell_scenarios.cpp)
  calcu_add_test(calcu_qt6_scenarios frequent COMMAND calcu_qt6_scenarios
                 PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
endif()

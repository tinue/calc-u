cmake_minimum_required(VERSION 3.21)
# Calc-U-59 Qt6 desktop app. Built as part of the repository's root CMake
# project (the root's tools/run_qt.sh, the local development build on
# macOS; the release packaging for Windows and Linux in CI,
# .github/workflows/calc-u-59.yml, in build/release). It does not configure
# on its own. The Mac Qt build is never shipped: on macOS the Swift app
# (Calc-U-59.xcodeproj) is the product.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "frontends/qt6 is part of the root CMake project: "
                      "configure the repository root, e.g. cmake -S ../../../.. -B ../../../../build/all")
endif()
#
# The app links calcu59_shared and calcu59_core of the product's
# CMakeLists.txt, the libraries the tests and the Swift app's XCFramework
# are built from. No GUI-independent
# logic lives in frontends/qt6/app; see CLAUDE.md § "Frontend Parity".
# The macOS minimum, C++20 and Homebrew's Qt prefix are the root project's.
project(CalcU59Qt LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
if(CALCU_ATTACH)   # calcu_qt6 links Qt Network for its attach line client
  find_package(Qt6 REQUIRED COMPONENTS Network)
endif()
qt_standard_project_setup()

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)  # the product directory
set(CORE_ROOT ${REPO_ROOT}/core)
set(SHARED_ROOT ${REPO_ROOT}/app)

# ---- Version: the single source is the Xcode project's MARKETING_VERSION ----
file(STRINGS ${REPO_ROOT}/frontends/swift/Calc-U-59.xcodeproj/project.pbxproj VERSION_LINES REGEX "MARKETING_VERSION = [0-9.]+;")
set(APP_VERSION "0.0.0")
foreach(line ${VERSION_LINES})
  if(line MATCHES "MARKETING_VERSION = ([0-9]+\\.[0-9.]+);")
    set(APP_VERSION ${CMAKE_MATCH_1})
    break()
  endif()
endforeach()

message(STATUS "Calc-U-59 (Qt) version ${APP_VERSION}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h @ONLY)
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.iss
     CONTENT "#define APP_VERSION \"@APP_VERSION@\"\n" @ONLY)

file(GLOB SHARED_SOURCES CONFIGURE_DEPENDS ${SHARED_ROOT}/*.cpp)
list(REMOVE_ITEM SHARED_SOURCES ${SHARED_ROOT}/HarnessProduct.cpp)   # the harness entry (calcu59_harness), not the app
set(CORE_SOURCES
  ${CORE_ROOT}/PC100C.cpp
  ${CORE_ROOT}/PC100CPaper.cpp
  ${CORE_ROOT}/SolidStateRomModule.cpp
  ${CORE_ROOT}/TI59Machine.cpp
  ${CORE_ROOT}/TMC0501.cpp
  ${CORE_ROOT}/ROM.cpp
  ${CORE_ROOT}/RAM.cpp
  ${CORE_ROOT}/TITypingSupport.cpp
  ${CORE_ROOT}/TIMachineState.cpp
  ${CORE_ROOT}/TIDebugging.cpp
  ${CORE_ROOT}/TIDebugKinds.cpp
  ${CORE_ROOT}/TICatalogue.cpp
  ${CORE_ROOT}/TIKeys.cpp
  ${CORE_ROOT}/TIMachine.cpp
  ${CORE_ROOT}/TIMemoryKinds.cpp
  ${CORE_ROOT}/TIMagneticCard.cpp
  ${CORE_ROOT}/TICardReaderPlace.cpp
  ${CORE_ROOT}/TIRomText.cpp
  ${CORE_ROOT}/TIRomReader.cpp
  ${CORE_ROOT}/ModuleFile.cpp
)

# The app's own sources but main.cpp: also CalcU59QtTests'.
set(CALCU59_QT_SOURCES
  app/MainWindow.cpp
  app/EmulatorController.cpp
  app/QtResourceBundle.cpp
  app/CalculatorPanel.cpp
  app/KeyboardWidget.cpp
  app/LedRenderer.cpp
  app/CueCardRenderer.cpp
  app/CardControls.cpp
  app/CardPickerDialog.cpp
  app/AppPaths.cpp
  app/debug/DebugPanel.cpp
  app/debug/LiveDebugWidget.cpp
  app/debug/CpuInspectorWidget.cpp
  app/debug/RomHeatmapWidget.cpp
  app/debug/AsmOverlayBox.cpp
  app/debug/LogWidget.cpp
  ${CORE_SOURCES}
  ${SHARED_SOURCES}
)
qt_add_executable(CalcU59Qt
  MACOSX_BUNDLE
  WIN32
  app/main.cpp
  ${CALCU59_QT_SOURCES}
  resources/calc-u-59.qrc
)
calcu_qt6_moc_compat(CalcU59Qt)

# ---- Shared assets, one source, no copies: the ROM files of roms/ (the 24
# of the full set, fetched by roms/fetch_roms.sh; the module files hold the
# cue cards) and the calculator and cue-card artwork from the Swift app's
# asset catalog. The app finds the ROMs through QtResourceBundle and the
# lookup ----
file(GLOB ROM_TXT ${REPO_ROOT}/roms/*.txt ${REPO_ROOT}/roms/*.rom.yaml)
foreach(f ${ROM_TXT})
  get_filename_component(name ${f} NAME)
  set_source_files_properties(${f} PROPERTIES QT_RESOURCE_ALIAS ${name})
endforeach()
qt_add_resources(CalcU59Qt "roms" PREFIX "/roms" FILES ${ROM_TXT})
set(CALCU59_QT_ROM_FILES ${ROM_TXT})

set(ARTWORK)
foreach(img ti59_base ti58_base CueCard MagnetCard SolidStateCard)
  set(f ${REPO_ROOT}/frontends/swift/App/Assets.xcassets/${img}.imageset/${img}.png)
  set_source_files_properties(${f} PROPERTIES QT_RESOURCE_ALIAS ${img}.png)
  list(APPEND ARTWORK ${f})
endforeach()
qt_add_resources(CalcU59Qt "artwork" PREFIX "/img" FILES ${ARTWORK})
set(CALCU59_QT_ARTWORK ${ARTWORK})

target_include_directories(CalcU59Qt PRIVATE ${CORE_ROOT} ${SHARED_ROOT} app ${CMAKE_CURRENT_BINARY_DIR}/generated)
# generated/BuildId.h: the build ID (tools/build-id.cmake), at each build.
calcu_build_id_header(CalcU59QtBuildId ${REPO_ROOT} CalcU59Qt)
# The machine interface: the core implements calcu::machine.
target_link_libraries(CalcU59Qt PRIVATE Qt6::Widgets calcu_library)
# The Qt6 component set: the paper panel, the run-control buttons and the
# clipboard service.
target_link_libraries(CalcU59Qt PRIVATE calcu_qt6)
# Harness attach in development and test builds
# (CALCU_ATTACH: POSIX trees with the tests; `--attach-port N`, else 59059),
# with the harness's product entry (app/HarnessProduct) that the attached
# harness runs, and its client in the same process, the pacing probe
# (`--pacing-probe <s>`). The release
# packaging (build/release) and Windows have neither. Checkpoint:
# CalcU59QtPacingProbe, one minute offscreen with a temporary HOME (no
# settings of the user's).
if(CALCU_ATTACH)
  target_sources(CalcU59Qt PRIVATE ${SHARED_ROOT}/HarnessProduct.cpp)
  set_source_files_properties(${SHARED_ROOT}/HarnessProduct.cpp PROPERTIES COMPILE_DEFINITIONS "CALCU59_VERSION=\"${APP_VERSION}\"")
  if(CALCU_TESTS AND CALCU_TESTS_CHECKPOINT)
    set(PROBE_HOME ${CMAKE_CURRENT_BINARY_DIR}/pacing-probe-home)
    file(MAKE_DIRECTORY ${PROBE_HOME})
    calcu_add_test(CalcU59QtPacingProbe checkpoint COMMAND $<TARGET_FILE:CalcU59Qt> --pacing-probe 60
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} PROPERTIES TIMEOUT 120)
    set_tests_properties(CalcU59QtPacingProbe PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${PROBE_HOME}")
  endif()
endif()
target_link_libraries(CalcU59Qt PRIVATE calcu_warnings)   # MSVC: /utf-8 for app/'s UTF-8 literals

if(WIN32)
  target_sources(CalcU59Qt PRIVATE resources/windows/calc-u-59.rc)
endif()

if(APPLE)
  # Local development bundle only (never signed, notarized or shipped).
  set(APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/AppIcon.icns)
  if(EXISTS ${APP_ICON})
    set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(CalcU59Qt PRIVATE ${APP_ICON})
  endif()
  set_target_properties(CalcU59Qt PROPERTIES
    MACOSX_BUNDLE_ICON_FILE AppIcon
    MACOSX_BUNDLE_GUI_IDENTIFIER ch.erzberger.Calc-U-59-Qt
    MACOSX_BUNDLE_BUNDLE_VERSION ${APP_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION}
    MACOSX_BUNDLE_BUNDLE_NAME "Calc-U-59"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/Info.plist.in
  )
endif()

set_target_properties(CalcU59Qt PROPERTIES OUTPUT_NAME "Calc-U-59")

# ---- Bundled example presets: the bundle
# file examples/bundle.yaml and exactly the files it lists, each `{ file: … }`,
# under Resources/examples in the macOS bundle, else resources/examples beside
# the program (AppPaths::bundledExamplesDir(); the preset list and the bundle
# paths `examples:<path>` read them). The Web build (frontends/web/build.js)
# and the Swift app take the same list. ----
set(EXAMPLES_SRC_DIR ${REPO_ROOT}/examples)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${EXAMPLES_SRC_DIR}/bundle.yaml)
file(STRINGS ${EXAMPLES_SRC_DIR}/bundle.yaml EXAMPLE_FILE_LINES REGEX "{ *file: ")
set(EXAMPLE_FILES "")
foreach(line IN LISTS EXAMPLE_FILE_LINES)
  string(REGEX REPLACE ".*{ *file: *([^ }]+) *}.*" "\\1" example_file "${line}")
  list(APPEND EXAMPLE_FILES "${example_file}")
endforeach()
list(REMOVE_DUPLICATES EXAMPLE_FILES)
if(APPLE)
  set_source_files_properties(${EXAMPLES_SRC_DIR}/bundle.yaml PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/examples")
  target_sources(CalcU59Qt PRIVATE ${EXAMPLES_SRC_DIR}/bundle.yaml)
endif()
foreach(example_file IN LISTS EXAMPLE_FILES)
  get_filename_component(example_dir "${example_file}" DIRECTORY)
  set(source ${EXAMPLES_SRC_DIR}/${example_file})
  if(APPLE)
    set_source_files_properties(${source} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/examples/${example_dir}")
    target_sources(CalcU59Qt PRIVATE ${source})
  else()
    add_custom_command(TARGET CalcU59Qt POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:CalcU59Qt>/resources/examples/${example_dir}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${source}
              "$<TARGET_FILE_DIR:CalcU59Qt>/resources/examples/${example_dir}"
      COMMENT "Copying the bundled example ${example_file}")
    install(FILES ${source} DESTINATION bin/resources/examples/${example_dir})
  endif()
endforeach()
if(NOT APPLE)
  add_custom_command(TARGET CalcU59Qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXAMPLES_SRC_DIR}/bundle.yaml
            "$<TARGET_FILE_DIR:CalcU59Qt>/resources/examples")
  install(FILES ${EXAMPLES_SRC_DIR}/bundle.yaml DESTINATION bin/resources/examples)
endif()

# CalcU59QtTests: the app's widgets
# on its EmulatorController, the session on the real ROMs (compiled in as
# the app has them), offscreen, with a HOME of its own (no settings of the
# user's are touched); the repository's one runner with a QApplication
# (library/qt6/tests/QtTestMain.cpp). On the real clock.
if(CALCU_TESTS)
  file(GLOB CALCU59_QT_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_tests.cpp)
  add_executable(CalcU59QtTests ${CALCU59_QT_SOURCES} ${CALCU59_QT_TEST_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/tests/QtFixture.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/../../../../library/qt6/tests/QtTestMain.cpp)
  qt_add_resources(CalcU59QtTests "roms" PREFIX "/roms" FILES ${CALCU59_QT_ROM_FILES})
  qt_add_resources(CalcU59QtTests "artwork" PREFIX "/img" FILES ${CALCU59_QT_ARTWORK})
  set_target_properties(CalcU59QtTests PROPERTIES AUTOMOC ON)
  calcu_qt6_moc_compat(CalcU59QtTests)
  add_dependencies(CalcU59QtTests CalcU59QtBuildId)
  target_include_directories(CalcU59QtTests PRIVATE ${CORE_ROOT} ${SHARED_ROOT} app tests ${CMAKE_CURRENT_BINARY_DIR}/generated
                             ${CMAKE_CURRENT_SOURCE_DIR}/../../../../library/testing)
  target_link_libraries(CalcU59QtTests PRIVATE Qt6::Widgets calcu_library calcu_qt6 calcu_warnings)
  target_compile_definitions(CalcU59QtTests PRIVATE CALCU59_DIR="${REPO_ROOT}" CALCU59_QT_TEST_OUT="${CMAKE_CURRENT_BINARY_DIR}/test-pictures")
  if(CALCU_ATTACH)   # the attach host in the test window (`qt_attach_drives_calc_u_59`)
    target_sources(CalcU59QtTests PRIVATE ${SHARED_ROOT}/HarnessProduct.cpp)
  endif()
  set(QT_TEST_HOME ${CMAKE_CURRENT_BINARY_DIR}/qt-tests-home)
  file(MAKE_DIRECTORY ${QT_TEST_HOME})
  calcu_add_test(CalcU59QtTests frequent COMMAND CalcU59QtTests)
  set_tests_properties(CalcU59QtTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${QT_TEST_HOME}")
endif()

# ---- Install (used by the Windows/Linux packaging jobs) ----
install(TARGETS CalcU59Qt BUNDLE DESTINATION . RUNTIME DESTINATION bin)

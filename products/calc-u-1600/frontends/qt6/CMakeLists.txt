cmake_minimum_required(VERSION 3.21)
# The Calc-U-1600 Qt6 app, a subdirectory of the product in the repository's
# root CMake project (the release packaging in CI builds it in
# build/release). It does not configure on its own.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "frontends/qt6 is part of the root CMake project: "
                      "configure the repository root, e.g. cmake -S ../../../.. -B ../../../../build/all")
endif()
# The minimum macOS for the .app is the root project's
# CMAKE_OSX_DEPLOYMENT_TARGET (15.8, set before its project()): it matches
# the vendored libsharpdx (Sharp-Data-Exchange's .cargo/config.toml) and
# tools/refresh_sharpdx.sh. C++20 and Homebrew's Qt prefix are the root's too.
if(APPLE)
  # The screenshot scenarios' MacShotSupport.mm needs Objective-C++ (the
  # paper's clipboard path and the macOS host settings are the Qt6 component
  # set's qt6/MacClipboard.mm and qt6/MacHost.mm).
  project(CalcU1600Qt CXX OBJCXX)
else()
  project(CalcU1600Qt CXX)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Network)
qt_standard_project_setup()

# ---- App version (About dialog, macOS bundle Info.plist) ----
# The one version: the generated Version.h below, the macOS bundle's
# versions (calcu_qt6_app further down) and the Windows installer (the
# generated Version.iss below) take it.
set(APP_VERSION "0.9.0")

message(STATUS "Calc-U-1600 version ${APP_VERSION}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app/Version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h @ONLY)
# Same version for the Windows installer: resources/windows/calc-u-1600.iss
# #includes this from the build dir for its AppVersion.
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.iss
     CONTENT "#define APP_VERSION \"@APP_VERSION@\"\n" @ONLY)

# core/ and app/ are the calcu1600_core / calcu1600_shared static libraries the
# product's CMakeLists.txt makes, with libsharpdx, which
# classifies and tokenizes every program the loaders read.

# MACOSX_BUNDLE: build a real .app on macOS (Contents/MacOS + Contents/
# Resources), not a bare executable -- needed both for normal macOS app
# behavior (Dock icon, Info.plist) and because the bundled .module.yaml
# resources below are placed under Contents/Resources, which only exists
# for a bundled target. WIN32 (plain CMake add_executable() keyword,
# passed through by qt_add_executable): link with the Windows GUI
# subsystem instead of the console one, so no terminal window opens
# alongside the app; qDebug/qWarning and the app log go to the app log's
# file (library/qt6/AppLogSink).
# The window's sources; the app adds main.cpp, CalcU1600QtTests its tests.
set(CALCU1600_QT_SOURCES
  app/MainWindow.cpp
  app/MachineController.cpp
  app/FaceplateWidget.cpp
  app/LcdWidget.cpp
  app/DebugPanel.cpp
  app/MediaControls.cpp
  app/Ce158PrinterWidget.cpp
  app/AppPaths.cpp
  app/MachineCodeLoadDialog.cpp
  app/AudioOutput.cpp
  app/debug/DapSocket.cpp
  resources/calc-u-1600.qrc)
qt_add_executable(CalcU1600Qt
  MACOSX_BUNDLE
  WIN32
  app/main.cpp
  ${CALCU1600_QT_SOURCES}
)

if(APPLE)
  target_link_libraries(CalcU1600Qt PRIVATE "-framework Cocoa")
endif()

if(WIN32)
  # Embeds the app icon into the .exe itself (Explorer/Start Menu/taskbar
  # pinned icon before launch) -- see resources/windows/calc-u-1600.rc.
  # Separate from QApplication::setWindowIcon() in app/main.cpp, which
  # only covers the running window.
  target_sources(CalcU1600Qt PRIVATE resources/windows/calc-u-1600.rc)
endif()

target_include_directories(CalcU1600Qt PRIVATE app ${CMAKE_CURRENT_BINARY_DIR}/generated)
# generated/BuildId.h (the build ID, the package) and BuildTime.cpp, at each build.
calcu_build_id_header(CalcU1600QtBuildId ${CMAKE_CURRENT_SOURCE_DIR}/../.. CalcU1600Qt)
target_link_libraries(CalcU1600Qt PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::Network)
# Harness attach in development and test builds (CALCU_ATTACH: POSIX trees
# with the tests), and its clients in the same process: the screenshot
# scenarios (`--shots`) and the pacing probe (`--pacing-probe <s>`).
# Release builds and Windows have neither.
if(CALCU_ATTACH)
  target_sources(CalcU1600Qt PRIVATE
    app/screenshots/ShotScenario.cpp
    app/screenshots/ShotCapture.cpp
    app/screenshots/ShotRunner.cpp
    app/screenshots/PacingProbe.cpp)
  target_link_libraries(CalcU1600Qt PRIVATE calcu1600_harness)   # the product the attached harness runs
  if(APPLE)
    target_sources(CalcU1600Qt PRIVATE app/screenshots/MacShotSupport.mm)
  endif()
endif()

target_link_libraries(CalcU1600Qt PRIVATE calcu1600_shared calcu_qt6)

# ---- Memory-card resource files, and bundled ROMs (loose, NOT .qrc --
# Core's scanMemoryCardDirectory() and the ROM lookup (DirectoryBundle)
# read a real filesystem directory, so these must be real files on
# disk) ----
set(CARD_RESOURCES_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/cards)
file(GLOB MODULE_FILES CONFIGURE_DEPENDS ${CARD_RESOURCES_SRC_DIR}/*.module.yaml)
# Bundled CE-1600F floppy disks (*.floppy.yaml, Connector/FloppyImageFile.hpp)
# -- listed in the disk picker ahead of the user's saved ones.
file(GLOB FLOPPY_FILES CONFIGURE_DEPENDS ${CARD_RESOURCES_SRC_DIR}/*.floppy.yaml)
# The bundled cassette template "Blank" (*.cassette.yaml with its sound).
file(GLOB CASSETTE_FILES CONFIGURE_DEPENDS ${CARD_RESOURCES_SRC_DIR}/*.cassette.yaml ${CARD_RESOURCES_SRC_DIR}/*.wav)
set(ROM_RESOURCES_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../roms)
# The ROM full set as roms/fetch_roms.sh fetches it: the dumps and the
# CE-502B's module definition.
file(GLOB BUNDLED_ROM_FILES CONFIGURE_DEPENDS ${ROM_RESOURCES_SRC_DIR}/*.ROM ${ROM_RESOURCES_SRC_DIR}/*.bin
     ${ROM_RESOURCES_SRC_DIR}/*.module.yaml)
# Calc-U-1600's own PC-1600 host-directory drive ROM: tracked in the repo
# (roms/ holds only the gitignored original dumps).
list(APPEND BUNDLED_ROM_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/pc1600-hostdrive/PC1600-P1-B7-HOSTDRIVE.bin)
set(BUNDLED_RESOURCE_FILES ${MODULE_FILES} ${FLOPPY_FILES} ${CASSETTE_FILES} ${BUNDLED_ROM_FILES})

# The app's data: inside the .app bundle Contents/Resources, else a
# "resources" folder next to the executable, so a plain `cmake --build`
# gives a runnable, self-contained build folder; share/calc-u-1600 when
# installed on Linux (AppPaths::bundledResourcesDir(), the library's
# calcu::qt6::bundledDataDir).
calcu_qt6_bundle_files(CalcU1600Qt data ${BUNDLED_RESOURCE_FILES})

# ---- The app's name, the bundled files every app carries (the examples of
# examples/bundle.yaml, the licences), the macOS bundle (the icon:
# resources/macos/AppIcon.icon compiled by actool into the committed
# Assets.car and AppIcon.icns; Info.plist.in with the document types) and
# the install rules: the library's calcu_qt6_app (library/qt6/CMakeLists.txt
# has the conventions and the layouts). Regenerate the icon files when the
# icon changes:
#   xcrun actool resources/macos/AppIcon.icon --compile resources/macos
#     --app-icon AppIcon --platform macosx --minimum-deployment-target 15.8
#     --target-device mac --output-partial-info-plist /dev/null
calcu_qt6_app(CalcU1600Qt NAME Calc-U-1600 VERSION ${APP_VERSION})

# CalcU1600QtTests, the window put together from the shell, its session on
# the real ROMs (a bundle folder of the tests' own), offscreen; the
# repository's one runner with a QApplication
# (library/qt6/tests/QtTestMain.cpp). Longer: on the real clock it takes
# about 25 s, too long for the Frequent budget; its ROMs-missing case and
# the attach check are Frequent.
if(CALCU_TESTS)
  find_package(Qt6 QUIET COMPONENTS Test)
  file(GLOB CALCU1600_QT_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_tests.cpp)
  add_executable(CalcU1600QtTests ${CALCU1600_QT_SOURCES} ${CALCU1600_QT_TEST_SOURCES}
                 ${CMAKE_CURRENT_SOURCE_DIR}/tests/QtFixture.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/../../../../library/qt6/tests/QtTestMain.cpp)
  if(CALCU_ATTACH)
    target_sources(CalcU1600QtTests PRIVATE app/screenshots/ShotScenario.cpp
                                            app/screenshots/ShotCapture.cpp app/screenshots/ShotRunner.cpp)
    if(APPLE)
      target_sources(CalcU1600QtTests PRIVATE app/screenshots/MacShotSupport.mm)
    endif()
    target_link_libraries(CalcU1600QtTests PRIVATE calcu1600_harness)
  endif()
  set_target_properties(CalcU1600QtTests PROPERTIES AUTOMOC ON AUTORCC ON)
  calcu_qt6_moc_compat(CalcU1600QtTests)
  calcu_use_build_id(CalcU1600QtBuildId CalcU1600QtTests)
  target_include_directories(CalcU1600QtTests PRIVATE app tests ${CMAKE_CURRENT_BINARY_DIR}/generated
                             ${CMAKE_CURRENT_SOURCE_DIR}/../../../../library/testing)
  target_link_libraries(CalcU1600QtTests PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::Network calcu1600_shared calcu_qt6)
  if(APPLE)
    target_link_libraries(CalcU1600QtTests PRIVATE "-framework Cocoa")
  endif()
  if(TARGET Qt6::Test)
    target_link_libraries(CalcU1600QtTests PRIVATE Qt6::Test)
    target_compile_definitions(CalcU1600QtTests PRIVATE CALCU_QT_TEST=1)
  endif()
  target_compile_definitions(CalcU1600QtTests PRIVATE "CALCU1600_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../..\"")
  calcu_add_test(CalcU1600QtTests longer COMMAND CalcU1600QtTests
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..
                 PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
  # A ROM folder without one file, in a process of its own (the bundle
  # folder is chosen once per process).
  calcu_add_test(CalcU1600QtTests.romsMissing frequent
                 COMMAND ${CMAKE_COMMAND} -E env CALCU1600_QT_ROMS_MISSING=1 $<TARGET_FILE:CalcU1600QtTests> roms_missing_in_window
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..
                 PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
endif()

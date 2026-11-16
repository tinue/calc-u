cmake_minimum_required(VERSION 3.21)
# The Calc-U-1600 Qt6 app, a subdirectory of the product in the repository's
# root CMake project (ADR 0003; the release packaging in CI builds it in
# build/release). It does not configure on its own.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR "frontends/qt6 is part of the root CMake project (ADR 0003): "
                      "configure the repository root, e.g. cmake -S ../../../.. -B ../../../../build/all")
endif()
# The minimum macOS for the .app is the root project's
# CMAKE_OSX_DEPLOYMENT_TARGET (15.8, set before its project()): it matches
# the vendored libsharpdx (SharpDataExchange's .cargo/config.toml) and
# tools/refresh_sharpdx.sh. C++20 and Homebrew's Qt prefix are the root's too.
if(APPLE)
  # MacAppSupport.mm and MacShotSupport.mm need Objective-C++ (the paper's
  # clipboard path is the Qt6 component set's qt6/MacClipboard.mm since M-45).
  project(CalcU1600Qt CXX OBJCXX)
else()
  project(CalcU1600Qt CXX)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Network)
qt_standard_project_setup()

# ---- App version / build id (About dialog, macOS bundle Info.plist) ----
# Single source of truth for the version number -- both the generated
# Version.h below and the MACOSX_BUNDLE_*VERSION* properties further down
# reference this instead of each hardcoding "0.1.0" separately, as does
# the Windows installer (generated Version.iss below).
set(APP_VERSION "0.9.0")

# git describe gives a short, human-meaningful build id (tag or short hash,
# +"-dirty" if the tree has local changes) for bug reports. Falls back to
# "unknown" when building from a source tree with no .git (e.g. a tarball
# produced by `git archive`) or when git itself isn't available -- this is
# a graceful degradation, not a build failure. Captured once at CMake
# configure time (not re-derived on every rebuild); re-run cmake to refresh
# it after checking out a different commit.
set(APP_BUILD_ID "unknown")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.git)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --always --dirty
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE APP_BUILD_ID_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_DESCRIBE_RESULT
    ERROR_QUIET
  )
  if(GIT_DESCRIBE_RESULT EQUAL 0 AND NOT APP_BUILD_ID_RAW STREQUAL "")
    set(APP_BUILD_ID ${APP_BUILD_ID_RAW})
  endif()
endif()
message(STATUS "Calc-U-1600 version ${APP_VERSION} build ${APP_BUILD_ID}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app/Version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h @ONLY)
# Same version for the Windows installer: resources/windows/calc-u-1600.iss
# #includes this from the build dir for its AppVersion.
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.iss
     CONTENT "#define APP_VERSION \"@APP_VERSION@\"\n" @ONLY)

# Core/ and Shared/ are the calcu_core / calcu_shared static libraries the
# product's CMakeLists.txt makes (cmake/CalcU.cmake), with libsharpdx, which
# classifies and tokenizes every program the loaders read.

# MACOSX_BUNDLE: build a real .app on macOS (Contents/MacOS + Contents/
# Resources), not a bare executable -- needed both for normal macOS app
# behavior (Dock icon, Info.plist) and because the bundled .card.yaml
# resources below are placed under Contents/Resources, which only exists
# for a bundled target. WIN32 (plain CMake add_executable() keyword,
# passed through by qt_add_executable): link with the Windows GUI
# subsystem instead of the console one, so no terminal window opens
# alongside the app -- see app/AppLogging.cpp for where qDebug/qWarning/
# etc. go instead now that there's no console to print them to.
# The window's sources; the app adds main.cpp, CalcU1600QtTests its tests.
set(CALCU1600_QT_SOURCES
  app/MainWindow.cpp
  app/MachineController.cpp
  app/FaceplateWidget.cpp
  app/LcdWidget.cpp
  app/DebugPanel.cpp
  app/MediaControls.cpp
  app/Ce158PrinterWidget.cpp
  app/HostKeyEvents.cpp
  app/AppPaths.cpp
  app/AppLogging.cpp
  app/MachineCodeLoadDialog.cpp
  app/AudioOutput.cpp
  app/debug/DapSocket.cpp
  resources/calc-u-1600.qrc)
if(APPLE)
  list(APPEND CALCU1600_QT_SOURCES app/MacAppSupport.mm)
endif()
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
target_link_libraries(CalcU1600Qt PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::Network)
# LIB-14 AC-44 (D.6): harness attach in development and test builds
# (CALCU_ATTACH: POSIX trees with the tests), and its clients in the same
# process: the screenshot scenarios (`--shots`) and the pacing probe
# (`--pacing-probe <s>`, LIB-17 §5). Release builds and Windows have neither.
if(CALCU_ATTACH)
  target_sources(CalcU1600Qt PRIVATE
    app/screenshots/AttachLineClient.cpp
    app/screenshots/ShotScenario.cpp
    app/screenshots/ShotCapture.cpp
    app/screenshots/ShotRunner.cpp
    app/screenshots/PacingProbe.cpp)
  target_link_libraries(CalcU1600Qt PRIVATE calcu1600_harness)   # the product the attached harness runs
  if(APPLE)
    target_sources(CalcU1600Qt PRIVATE app/screenshots/MacShotSupport.mm)
  endif()
endif()

target_link_libraries(CalcU1600Qt PRIVATE calcu_shared calcu_qt6)

# ---- App icon (macOS bundle icon; Windows' equivalent is the WIN32
# block's calc-u-1600.rc above, Linux's is the AppImage's --icon-file in
# build.yml). The source is the Icon Composer bundle
# resources/macos/AppIcon.icon (macOS 26+ masks, sizes and shades it). The CI runner's Xcode can't compile .icon, so its actool
# output is committed, same as the vendored libsharpdx.a above: Assets.car
# (macOS 26+) and AppIcon.icns (fallback for older macOS). Regenerate both
# if the icon changes:
#   xcrun actool Qt6/resources/macos/AppIcon.icon --compile Qt6/resources/macos
#     --app-icon AppIcon --platform macosx --minimum-deployment-target 15.8
#     --target-device mac --output-partial-info-plist /dev/null
if(APPLE)
  set(APP_ICON_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/AppIcon.icns
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/Assets.car)
  set_source_files_properties(${APP_ICON_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(CalcU1600Qt PRIVATE ${APP_ICON_FILES})
  # Explicit bundle identifier (same ch.erzberger.* namespace as
  # SharpDataExchange's ch.erzberger.SharpDataExchange) -- needed for
  # stable codesigning/notarization identity across CI runs, rather than
  # whatever default CMake would otherwise synthesize.
  set_target_properties(CalcU1600Qt PROPERTIES
    MACOSX_BUNDLE_ICON_FILE AppIcon
    MACOSX_BUNDLE_GUI_IDENTIFIER ch.erzberger.CalcU1600Qt
    MACOSX_BUNDLE_BUNDLE_VERSION ${APP_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION}
    MACOSX_BUNDLE_BUNDLE_NAME "Calc-U-1600"
    # Qt's template plus the document types the Dock and Finder open here.
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/Info.plist.in
  )
endif()

# User-facing output name: "Calc-U-1600" everywhere (the .app bundle on
# macOS, the plain executable on Linux/Windows) -- the CMake target
# identifier above stays CalcU1600Qt (internal only, not user-visible;
# renaming it would mean touching every target_* call in this file for
# no user-facing benefit).
set_target_properties(CalcU1600Qt PROPERTIES OUTPUT_NAME "Calc-U-1600")

# ---- Memory-card resource files, and bundled ROMs (loose, NOT .qrc --
# Core's scanMemoryCardDirectory() and the ROM lookup (DirectoryBundle,
# LIB-12) read a real filesystem directory, so these must be real files on
# disk) ----
set(CARD_RESOURCES_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/cards)
file(GLOB CARD_YAML_FILES ${CARD_RESOURCES_SRC_DIR}/*.card.yaml)
# Bundled CE-1600F floppy disks (*.floppy.yaml, Connector/FloppyImageFile.hpp)
# -- listed in the disk picker ahead of the user's saved ones.
file(GLOB FLOPPY_FILES ${CARD_RESOURCES_SRC_DIR}/*.floppy.yaml)
# The bundled tape template "Empty" (*.tape.yaml with its sound, LIB-7 AC-42, D-23).
file(GLOB TAPE_FILES ${CARD_RESOURCES_SRC_DIR}/*.tape.yaml ${CARD_RESOURCES_SRC_DIR}/*.wav)
set(ROM_RESOURCES_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../roms)
# The ROM full set as roms/fetch_roms.sh fetches it: the dumps and the
# CE-502B's module definition (LIB-12 AC-12, AC-14).
file(GLOB BUNDLED_ROM_FILES ${ROM_RESOURCES_SRC_DIR}/*.ROM ${ROM_RESOURCES_SRC_DIR}/*.bin
     ${ROM_RESOURCES_SRC_DIR}/*.card.yaml)
# Calc-U-1600's own PC-1600 host-directory drive ROM: tracked in the repo
# (roms/ holds only the gitignored original dumps).
list(APPEND BUNDLED_ROM_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/pc1600-hostdrive/PC1600-P1-B7-HOSTDRIVE.bin)
set(BUNDLED_RESOURCE_FILES ${CARD_YAML_FILES} ${FLOPPY_FILES} ${TAPE_FILES} ${BUNDLED_ROM_FILES})

if(APPLE)
  # Inside the .app bundle: Contents/Resources -- AppPaths::
  # bundledResourcesDir() resolves applicationDirPath()/../Resources
  # (Contents/MacOS/../Resources) to land here.
  set_source_files_properties(${BUNDLED_RESOURCE_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(CalcU1600Qt PRIVATE ${BUNDLED_RESOURCE_FILES})
else()
  # Windows/Linux: a "resources" directory copied next to the built
  # executable at build time, so a plain `cmake --build` produces a
  # runnable, self-contained build dir.
  add_custom_command(TARGET CalcU1600Qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:CalcU1600Qt>/resources"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${BUNDLED_RESOURCE_FILES}
            "$<TARGET_FILE_DIR:CalcU1600Qt>/resources"
    COMMENT "Copying bundled .card.yaml/ROM resources next to the executable")
  install(FILES ${BUNDLED_RESOURCE_FILES} DESTINATION resources)
endif()

# ---- Bundled example presets (C1600-4 AC-17, LIB-6 AC-31; row 1600-21):
# the bundle file examples/bundle.yaml and exactly the files it lists, each
# `{ file: … }` (the presets and their files), loose under Resources/examples
# (AppPaths::bundledExamplesDir(); the preset list reads them). The Web build
# (frontends/web/build.js) takes the same list. ----
set(EXAMPLES_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../examples)
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
  target_sources(CalcU1600Qt PRIVATE ${EXAMPLES_SRC_DIR}/bundle.yaml)
endif()
foreach(example_file IN LISTS EXAMPLE_FILES)
  get_filename_component(example_dir "${example_file}" DIRECTORY)
  set(source ${EXAMPLES_SRC_DIR}/${example_file})
  if(APPLE)
    set_source_files_properties(${source} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/examples/${example_dir}")
    target_sources(CalcU1600Qt PRIVATE ${source})
  else()
    add_custom_command(TARGET CalcU1600Qt POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:CalcU1600Qt>/resources/examples/${example_dir}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${source}
              "$<TARGET_FILE_DIR:CalcU1600Qt>/resources/examples/${example_dir}"
      COMMENT "Copying the bundled example ${example_file}")
    install(FILES ${source} DESTINATION resources/examples/${example_dir})
  endif()
endforeach()
if(NOT APPLE)
  add_custom_command(TARGET CalcU1600Qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXAMPLES_SRC_DIR}/bundle.yaml
            "$<TARGET_FILE_DIR:CalcU1600Qt>/resources/examples")
  install(FILES ${EXAMPLES_SRC_DIR}/bundle.yaml DESTINATION resources/examples)
endif()

# ---- License / third-party-notice files -- ships GPLv3 (this project)
# and LGPLv3 (bundled Qt6) license text alongside the binary, so LGPLv3's
# obligation to provide the license text travels with every distributed
# build, not just the git repo. See ../THIRD-PARTY-NOTICES.md.
set(LICENSE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../LICENSE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../NOTICE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../THIRD-PARTY-NOTICES.md
  ${CMAKE_CURRENT_SOURCE_DIR}/../../licenses/LGPL-3.0.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/../../licenses/GPL-3.0.txt
)

if(APPLE)
  set_source_files_properties(${LICENSE_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/licenses")
  target_sources(CalcU1600Qt PRIVATE ${LICENSE_FILES})
else()
  add_custom_command(TARGET CalcU1600Qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:CalcU1600Qt>/licenses"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LICENSE_FILES}
            "$<TARGET_FILE_DIR:CalcU1600Qt>/licenses"
    COMMENT "Copying license/third-party-notice files next to the executable")
  install(FILES ${LICENSE_FILES} DESTINATION licenses)
endif()

# LIB-14 Part D (D.8): CalcU1600QtTests, the window put together from the
# shell, its session on the real ROMs (a bundle folder of the tests' own),
# offscreen; the repository's one runner with a QApplication
# (library/qt6/tests/QtTestMain.cpp). Longer: on the real clock it takes
# about 25 s (the design's Frequent did not fit the budget, M-49 qa-report);
# its ROMs-missing case and the attach check are Frequent.
if(CALCU_TESTS)
  find_package(Qt6 QUIET COMPONENTS Test)
  file(GLOB CALCU1600_QT_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_tests.cpp)
  add_executable(CalcU1600QtTests ${CALCU1600_QT_SOURCES} ${CALCU1600_QT_TEST_SOURCES}
                 ${CMAKE_CURRENT_SOURCE_DIR}/tests/QtFixture.cpp
                 ${CMAKE_CURRENT_SOURCE_DIR}/../../../../library/qt6/tests/QtTestMain.cpp)
  if(CALCU_ATTACH)
    target_sources(CalcU1600QtTests PRIVATE app/screenshots/AttachLineClient.cpp app/screenshots/ShotScenario.cpp
                                            app/screenshots/ShotCapture.cpp app/screenshots/ShotRunner.cpp)
    if(APPLE)
      target_sources(CalcU1600QtTests PRIVATE app/screenshots/MacShotSupport.mm)
    endif()
    target_link_libraries(CalcU1600QtTests PRIVATE calcu1600_harness)
  endif()
  set_target_properties(CalcU1600QtTests PROPERTIES AUTOMOC ON AUTORCC ON)
  calcu_qt6_moc_compat(CalcU1600QtTests)
  target_include_directories(CalcU1600QtTests PRIVATE app tests ${CMAKE_CURRENT_BINARY_DIR}/generated
                             ${CMAKE_CURRENT_SOURCE_DIR}/../../../../library/testing)
  target_link_libraries(CalcU1600QtTests PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::Network calcu_shared calcu_qt6)
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
  # AC-19: a ROM folder without one file, in a process of its own (the
  # bundle folder is chosen once per process).
  calcu_add_test(CalcU1600QtTests.romsMissing frequent
                 COMMAND ${CMAKE_COMMAND} -E env CALCU1600_QT_ROMS_MISSING=1 $<TARGET_FILE:CalcU1600QtTests> lib14_ac19
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..
                 PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
  # D.8 Q12: the shot runner steps the machine through attach only.
  calcu_add_test(CalcU1600ShotsThroughAttach frequent
                 COMMAND ${CMAKE_COMMAND} -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/app/screenshots/ShotRunner.cpp
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_shots_through_attach.cmake)
endif()

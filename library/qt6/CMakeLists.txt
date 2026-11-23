# The Qt6 component set:
# the library's widgets for the Qt6 frontends, in namespace calcu::qt6. A
# static target of its own, calcu_qt6, because calcu_library stays free of Qt
# for the Web and Swift builds (an exception to the one library target). Built
# only with CALCU_FRONTEND_QT and when Qt6 is found, never under Emscripten or
# for iOS (library/CMakeLists.txt).
#
# Its tests: one program, calcu_qt6_tests, every library/qt6/tests/*_tests.cpp
# on the repository's one runner with a QApplication; Frequent, on the
# offscreen platform. Beside it calcu_qt6_scenarios, the shell's GUI
# scenarios on the test window.
find_package(Qt6 QUIET COMPONENTS Widgets)
if(NOT Qt6_FOUND)
  message(STATUS "Qt6 not found: no library Qt6 component set (calcu_qt6)")
  return()
endif()

add_library(calcu_qt6 STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/PaperPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ChromeStyle.cpp          # stylesheets over a product's chrome
  ${CMAKE_CURRENT_SOURCE_DIR}/PaperPainter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ClipboardService.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/RunControlButtons.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MediaPicker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/NameAndSaveDialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/AskedSend.cpp)           # the `asks:` round trip of media commands
# The shell.
target_sources(calcu_qt6 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/SessionHost.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/HostFolders.cpp          # settings and folders on a Qt6 host
  ${CMAKE_CURRENT_SOURCE_DIR}/BundledData.cpp          # where an app finds the files it carries
  ${CMAKE_CURRENT_SOURCE_DIR}/FileOpening.cpp          # opening files on a Qt6 host
  ${CMAKE_CURRENT_SOURCE_DIR}/Shell.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellMenus.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellControlBar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MediaPlaceControl.cpp    # one media place on the bar
  ${CMAKE_CURRENT_SOURCE_DIR}/SlotPicker.cpp           # a module slot's picker on the bar
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellSettingsDialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellLoading.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellNotices.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShellAbout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PanelArea.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/FaceplateInput.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/HostKeys.cpp          # host keys to host-key-down
  ${CMAKE_CURRENT_SOURCE_DIR}/RomsMissingView.cpp)
# The in-process line client of harness attach, for
# the products' attach clients (Calc-U-1600's screenshot scenarios, both
# pacing probes); only with CALCU_ATTACH, so only those trees need Qt Network.
if(CALCU_ATTACH)
  find_package(Qt6 REQUIRED COMPONENTS Network)
  target_sources(calcu_qt6 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/AttachLineClient.cpp
                                   ${CMAKE_CURRENT_SOURCE_DIR}/PacingProbe.cpp)
  target_link_libraries(calcu_qt6 PUBLIC Qt6::Network)
endif()
# On macOS the picture of `clipboard-image` goes through
# Cocoa, which keeps its physical size (Qt's clipboard loses it); MacHost
# holds the host settings both apps apply at their start.
if(APPLE)
  target_sources(calcu_qt6 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/MacClipboard.mm
                                   ${CMAKE_CURRENT_SOURCE_DIR}/MacHost.mm)
  target_link_libraries(calcu_qt6 PUBLIC "-framework Cocoa")
endif()
set_target_properties(calcu_qt6 PROPERTIES AUTOMOC ON)
target_link_libraries(calcu_qt6 PUBLIC calcu_library Qt6::Widgets PRIVATE calcu_warnings)
target_include_directories(calcu_qt6 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(calcu_qt6 PUBLIC cxx_std_20)
# Qt before 6.5 (Ubuntu 24.04's 6.4.2): its moc cannot parse `concept`, and
# with the target's own -std=gnu++20 AutoMoc's compiler predefines make it
# read libstdc++ 13's <concepts> (seen on CI). Without the predefines moc skips
# those sections, as it does for the product apps, whose predefines are
# taken without -std.
function(calcu_qt6_moc_compat target)
  if(Qt6_VERSION VERSION_LESS 6.5)
    set_target_properties(${target} PROPERTIES AUTOMOC_COMPILER_PREDEFINES OFF)
  endif()
endfunction()
calcu_qt6_moc_compat(calcu_qt6)

# The files a Qt6 app carries beside its program, in the build tree and in
# an install. CMake's own copies (MACOSX_PACKAGE_LOCATION, POST_BUILD
# copies) only add files, never remove one; these keep the app's folders in
# the build tree at exactly what the lists name, at each build.
#
#   calcu_qt6_bundle_files(<target> <place> <files>...)
#     <place> is `data` or `data/<folder>` (the app's bundled data:
#     examples, module definitions, ROM images, the macOS icon) or
#     `licenses` (the licence and notice files).
#   calcu_qt6_bundle_sync(<target> DESKTOP_FILE <file> ICON <png>)
#     after the last calcu_qt6_bundle_files: the target <target>_bundle,
#     run before each build of the app (BundleSync.cmake), owns the build
#     tree's folders of the places and removes what is not listed there;
#     and the install rules of the app: the program, its bundled files and,
#     on Linux, the .desktop file and the icon (a square PNG, its size read
#     from the file).
#
# With <App> the target's OUTPUT_NAME and <app> the same in lower case:
#
#                 build tree              cmake --install, under the prefix
#   macOS         <App>.app/Contents/     <App>.app as a whole
#     data        Resources/
#     licenses    Resources/licenses/
#   Linux         <program's folder>/     bin/<App>
#     data        resources/              share/<app>/
#     licenses    licenses/               share/doc/<app>/
#                                         share/applications/<DESKTOP_FILE>,
#                                         share/icons/hicolor/<n>x<n>/apps/<ICON>
#   Windows       <program's folder>\     <App>\<App>.exe with Qt's DLLs (windeployqt)
#     data        resources\              <App>\resources\
#     licenses    licenses\               <App>\licenses\
#
# The program finds its data through calcu::qt6::bundledDataDir
# (library/qt6/BundledData). An install on macOS and Linux uses the Qt the
# app was built with (its library folders are the installed program's
# rpath); the packages (DMG, AppImage) bring their own. Windows has no Qt
# of the system, so its install deploys Qt beside the program.
function(calcu_qt6_bundle_files target place)
  string(REGEX REPLACE "/+$" "" place "${place}")   # "data/examples/" of a file at the top
  if(NOT place MATCHES "^(data(/.+)?|licenses)$")
    message(FATAL_ERROR "calcu_qt6_bundle_files(${target}): the place ${place} is not data[/<folder>] or licenses")
  endif()
  foreach(f IN LISTS ARGN)
    get_filename_component(f "${f}" ABSOLUTE)
    set_property(TARGET ${target} APPEND PROPERTY CALCU_BUNDLE_FILES "${place}|${f}")
  endforeach()
endfunction()

function(calcu_qt6_bundle_sync target)
  cmake_parse_arguments(B "" "DESKTOP_FILE;ICON" "" ${ARGN})
  if(NOT B_DESKTOP_FILE OR NOT B_ICON)
    message(FATAL_ERROR "calcu_qt6_bundle_sync(${target}): DESKTOP_FILE and ICON are needed")
  endif()
  get_target_property(App ${target} OUTPUT_NAME)
  if(NOT App)
    set(App ${target})
  endif()
  string(TOLOWER "${App}" app)
  # The build tree's and the install's folder of each top place.
  if(APPLE)
    set(build_data Resources)
    set(build_licenses Resources/licenses)
  else()
    set(build_data resources)
    set(build_licenses licenses)
  endif()
  if(WIN32)
    set(install_data ${App}/resources)
    set(install_licenses ${App}/licenses)
  else()
    set(install_data ${CMAKE_INSTALL_DATADIR}/${app})
    set(install_licenses ${CMAKE_INSTALL_DATADIR}/doc/${app})
  endif()

  get_property(entries TARGET ${target} PROPERTY CALCU_BUNDLE_FILES)
  set(manifest "")
  set(dests "")
  foreach(entry IN LISTS entries)
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 place)
    list(GET parts 1 source)
    if(place STREQUAL "licenses")
      set(build_place ${build_licenses})
      set(install_place ${install_licenses})
    else()
      string(REGEX REPLACE "^data" "" sub "${place}")
      set(build_place ${build_data}${sub})
      set(install_place ${install_data}${sub})
    endif()
    get_filename_component(name "${source}" NAME)
    if("${build_place}/${name}" IN_LIST dests)
      message(FATAL_ERROR "calcu_qt6_bundle_sync(${target}): two files for ${build_place}/${name}")
    endif()
    list(APPEND dests "${build_place}/${name}")
    string(APPEND manifest "${build_place}|${source}\n")
    if(NOT APPLE)   # the bundle installs as a whole
      install(FILES ${source} DESTINATION ${install_place})
    endif()
  endforeach()
  # Written only when it changes; a list changes at configure time
  # (CMAKE_CONFIGURE_DEPENDS, CONFIGURE_DEPENDS globs).
  set(manifest_file ${CMAKE_CURRENT_BINARY_DIR}/${target}_bundle.txt)
  file(CONFIGURE OUTPUT ${manifest_file} CONTENT "${manifest}" @ONLY)
  if(APPLE)
    set(root "$<TARGET_BUNDLE_CONTENT_DIR:${target}>")
  else()
    set(root "$<TARGET_FILE_DIR:${target}>")
  endif()
  # Without outputs the target is out of date at every build: a file put
  # into the app by hand goes too. An unchanged app costs a look at the
  # times of its files.
  add_custom_target(${target}_bundle
    COMMAND ${CMAKE_COMMAND} -DMANIFEST=${manifest_file} -DROOT=${root}
            -P ${CALCU_QT6_DIR}/BundleSync.cmake
    COMMENT "Bundled files of ${target}"
    VERBATIM)
  add_dependencies(${target} ${target}_bundle)

  # The program.
  if(APPLE)
    install(TARGETS ${target} BUNDLE DESTINATION .)
  elseif(WIN32)
    install(TARGETS ${target} RUNTIME DESTINATION ${App})
  else()
    install(TARGETS ${target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
  if(NOT WIN32)
    # The installed program keeps the Qt it was linked with (Homebrew's,
    # the distribution's lies in a default folder and needs none).
    set_target_properties(${target} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
  endif()
  if(WIN32)
    # The flags of the packaging before Inno Setup: the MSVC runtime's
    # installer comes along (the .iss runs it).
    find_program(CALCU_WINDEPLOYQT windeployqt HINTS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}" REQUIRED)
    install(CODE "execute_process(COMMAND \"${CALCU_WINDEPLOYQT}\" $<IF:$<CONFIG:Debug>,--debug,--release> --compiler-runtime
                    \"\${CMAKE_INSTALL_PREFIX}/${App}/$<TARGET_FILE_NAME:${target}>\" COMMAND_ERROR_IS_FATAL ANY)")
  elseif(NOT APPLE)
    install(FILES ${B_DESKTOP_FILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    # The PNG's width, bytes 16 to 19 of its header.
    file(READ ${B_ICON} width OFFSET 16 LIMIT 4 HEX)
    math(EXPR size "0x${width}")
    install(FILES ${B_ICON} DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps)
  endif()
endfunction()
set(CALCU_QT6_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

# Where `cmake --install` puts the apps when no --prefix is given: a
# folder of the user's, without root. macOS ~/Applications (the .app),
# Linux ~/.local (bin/, share/), Windows %LOCALAPPDATA%\Programs (one folder
# per app).
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(APPLE)
    set(prefix "$ENV{HOME}/Applications")
  elseif(WIN32)
    file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}/Programs" prefix)
  else()
    set(prefix "$ENV{HOME}/.local")
  endif()
  set(CMAKE_INSTALL_PREFIX "${prefix}" CACHE PATH "Where cmake --install puts the Qt6 apps" FORCE)
endif()
include(GNUInstallDirs)
message(STATUS "Qt6 apps install into ${CMAKE_INSTALL_PREFIX}")

if(CALCU_TESTS)
  # QTest's key and mouse clicks for the shortcut cases; a Qt
  # without the Test module skips them.
  find_package(Qt6 QUIET COMPONENTS Test)
  # The shell's test window over TestShellPart (library/shell/tests/) and,
  # with CALCU_ATTACH, the harness's test product for the attach host.
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
  # The shell's GUI scenarios, a program of their own.
  calcu_qt6_test_program(calcu_qt6_scenarios ${CMAKE_CURRENT_SOURCE_DIR}/tests/shell_scenarios.cpp)
  calcu_add_test(calcu_qt6_scenarios frequent COMMAND calcu_qt6_scenarios
                 PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
endif()

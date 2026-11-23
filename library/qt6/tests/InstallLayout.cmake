# The layout `cmake --install` gives the Qt6 apps of a tree
# (calcu_qt6_app, library/qt6/CMakeLists.txt): installs the tree
# into a folder of its own and looks for each app's program, bundled data
# and licences, on Linux also the .desktop file, the icon and the presets'
# MIME type, on Windows Qt's DLLs. CTest LibraryQtInstallLayout (Frequent, Qt trees).
#
#   cmake -DBUILD=<tree> -DPREFIX=<folder> -DAPPS=<App>,<App> -DIS_APPLE=… -DIS_WIN32=…
#         -P library/qt6/tests/InstallLayout.cmake
cmake_minimum_required(VERSION 3.21)
foreach(v BUILD PREFIX APPS)
  if(NOT DEFINED ${v})
    message(FATAL_ERROR "InstallLayout.cmake: -D${v}=… is needed")
  endif()
endforeach()

string(REPLACE "," ";" APPS "${APPS}")
file(REMOVE_RECURSE "${PREFIX}")
execute_process(COMMAND ${CMAKE_COMMAND} --install "${BUILD}" --prefix "${PREFIX}"
                OUTPUT_QUIET RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "cmake --install ${BUILD} failed (${rc})")
endif()

set(missing "")
macro(expect path)
  if(NOT EXISTS "${PREFIX}/${path}")
    list(APPEND missing "${path}")
  endif()
endmacro()

foreach(App IN LISTS APPS)
  string(TOLOWER "${App}" app)
  if(IS_APPLE)
    expect(${App}.app/Contents/MacOS/${App})
    expect(${App}.app/Contents/Resources/examples/bundle.yaml)
    expect(${App}.app/Contents/Resources/licenses/LICENSE)
  elseif(IS_WIN32)
    expect(${App}/${App}.exe)
    expect(${App}/resources/examples/bundle.yaml)
    expect(${App}/licenses/LICENSE)
    file(GLOB qt_core "${PREFIX}/${App}/Qt6Core*.dll")
    if(NOT qt_core)
      list(APPEND missing "${App}/Qt6Core.dll")
    endif()
  else()
    expect(bin/${App})
    expect(share/${app}/examples/bundle.yaml)
    expect(share/doc/${app}/LICENSE)
    expect(share/applications/${app}.desktop)
    expect(share/icons/hicolor/256x256/apps/${app}.png)
    expect(share/mime/packages/${app}.xml)
  endif()
endforeach()

if(missing)
  list(JOIN missing "\n  " shown)
  message(FATAL_ERROR "Not installed under ${PREFIX}:\n  ${shown}")
endif()
message(STATUS "Installed as laid out: ${APPS}")

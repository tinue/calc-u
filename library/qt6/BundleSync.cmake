# Brings a Qt6 app's bundled folders to exactly the files its manifest lists
# (calcu_qt6_bundle_files, library/qt6/CMakeLists.txt), at each build of the
# app:
#
#   cmake -DMANIFEST=<file> -DROOT=<folder> -P library/qt6/BundleSync.cmake
#
# ROOT is the folder the manifest's places are relative to (the .app's
# Contents, else the program's folder). Each manifest line is
# `<place>|<source>`: the source goes into ROOT/<place>. A file is copied
# when it is missing or older than its source, so a build that changed
# nothing only compares times. Every other file and folder under the
# manifest's top folders (the first part of each place) is removed: a file
# dropped from the source's lists leaves the app at its next build.
if(NOT MANIFEST OR NOT ROOT)
  message(FATAL_ERROR "BundleSync.cmake: -DMANIFEST=<file> -DROOT=<folder> are needed")
endif()

file(STRINGS ${MANIFEST} lines)
set(wanted "")
set(wanted_dirs "")
set(tops "")
foreach(line IN LISTS lines)
  string(FIND "${line}" "|" bar)
  string(SUBSTRING "${line}" 0 ${bar} place)
  math(EXPR after "${bar} + 1")
  string(SUBSTRING "${line}" ${after} -1 source)
  if(NOT EXISTS "${source}")
    message(FATAL_ERROR "BundleSync.cmake: ${source} (for ${place}) does not exist")
  endif()
  get_filename_component(name "${source}" NAME)
  set(dest "${ROOT}/${place}/${name}")
  if(NOT EXISTS "${dest}" OR "${source}" IS_NEWER_THAN "${dest}")
    file(MAKE_DIRECTORY "${ROOT}/${place}")
    file(COPY_FILE "${source}" "${dest}")
  endif()
  list(APPEND wanted "${dest}")
  # The folders on the way to the file stay.
  set(dir "${place}")
  while(NOT dir STREQUAL "")
    list(APPEND wanted_dirs "${ROOT}/${dir}")
    get_filename_component(dir "${dir}" DIRECTORY)
  endwhile()
  string(REGEX REPLACE "/.*" "" top "${place}")
  list(APPEND tops "${top}")
endforeach()
list(REMOVE_DUPLICATES tops)

foreach(top IN LISTS tops)
  file(GLOB_RECURSE present LIST_DIRECTORIES true "${ROOT}/${top}/*")
  # Deepest first, so a folder is emptied before it is looked at.
  list(SORT present ORDER DESCENDING)
  foreach(path IN LISTS present)
    if(IS_DIRECTORY "${path}" AND NOT IS_SYMLINK "${path}")
      if(NOT path IN_LIST wanted_dirs)
        file(REMOVE_RECURSE "${path}")
      endif()
    elseif(NOT path IN_LIST wanted)
      file(REMOVE "${path}")
    endif()
  endforeach()
endforeach()

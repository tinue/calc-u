# calcu_add_preset_check(<name> PRODUCT <word> FILES <glob>… [EXCLUDE <file name>…] [FILL <script>])
#
# One CTest entry, calcu_preset_check.<name> (Frequent), that runs `calcu
# --product <word> check-preset` over the files: the run fails when a file
# fails. FILES are globs relative to the calling directory; EXCLUDE names
# files by their file name (the presets that wait for a later feature, by
# name); FILL is a CMake script run as `cmake -DIN=<file> -DOUT=<file> -P
# <script>` on each file first, which writes the file with its placeholders
# filled in (the VS Code templates of Calc-U-1600). The product `test` is
# `calcu_test`, every other `calcu`.
set(CALCU_PRESETS_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")

function(calcu_add_preset_check name)
  cmake_parse_arguments(P "" "PRODUCT;FILL" "FILES;EXCLUDE" ${ARGN})
  set(files "")
  foreach(pattern ${P_FILES})
    file(GLOB found ${CMAKE_CURRENT_SOURCE_DIR}/${pattern})
    list(APPEND files ${found})
  endforeach()
  set(kept "")
  foreach(f ${files})
    get_filename_component(fname ${f} NAME)
    list(FIND P_EXCLUDE ${fname} at)
    if(at LESS 0)
      list(APPEND kept ${f})
    endif()
  endforeach()
  list(SORT kept)
  if(NOT kept)
    message(FATAL_ERROR "calcu_add_preset_check(${name}): no file")
  endif()
  if(P_PRODUCT STREQUAL "test")
    set(program $<TARGET_FILE:calcu_test>)
  else()
    set(program $<TARGET_FILE:calcu>)
  endif()
  string(REPLACE ";" "|" joined "${kept}")
  set(fill "")
  if(P_FILL)
    set(fill ${CMAKE_CURRENT_SOURCE_DIR}/${P_FILL})
  endif()
  calcu_add_test(calcu_preset_check.${name} frequent
                 COMMAND ${CMAKE_COMMAND} -DCALCU=${program} -DPRODUCT=${P_PRODUCT} "-DFILES=${joined}"
                         "-DFILL=${fill}" -DWORK=${CMAKE_CURRENT_BINARY_DIR}/preset-check-${name}
                         -P ${CALCU_PRESETS_DIR}/RunPresetCheck.cmake)
endfunction()

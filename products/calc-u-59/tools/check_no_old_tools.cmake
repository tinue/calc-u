# LIB-11 Part E (design §11.6 H4; plan M-59 "Green after": no old tool and no
# adapter of Calc-U-59 left): tools/headless/ and tools/agentic/ are gone,
# with the target ti59headless and its library variable TI59HEADLESS_LIB,
# and no file is named for adapters (core/TIAdapters.* went, D-17). Their
# functions are `calcu --product c59` commands (LIB-11 AC-14, C59-9 §4); the
# scripts that stay are tools/coverage.py, tools/fastmode_matrix.py and
# tools/calendar/. Run from products/calc-u-59 with
# `cmake -P tools/check_no_old_tools.cmake` (CTest CalcU59NoOldTools).

set(here ${CMAKE_CURRENT_LIST_DIR}/..)
set(gone tools/headless tools/agentic core/TIAdapters.hpp core/TIAdapters.cpp tests/headless_rom_tests.py)
set(status 0)
foreach(f ${gone})
  if(EXISTS ${here}/${f})
    message("${f} is still there (M-59 removed it).")
    set(status 1)
  endif()
endforeach()

# Nothing builds or names the old library.
file(GLOB_RECURSE sources ${here}/CMakeLists.txt ${here}/frontends/qt6/CMakeLists.txt
                          ${here}/core/*.cpp ${here}/core/*.hpp ${here}/app/*.cpp ${here}/app/*.hpp
                          ${here}/tests/*.cpp ${here}/tests/*.hpp ${here}/tests/*.sh ${here}/tests/*.py
                          ${here}/tools/*.py ${here}/tools/*.sh ${here}/tools/calendar/*.py)
foreach(f ${sources})
  file(READ ${f} text)
  foreach(word ti59headless TI59HEADLESS_LIB "#include[^\n]*TIAdapters\\.hpp" "memoryOf\\(" TIMemoryAdapter)
    if(text MATCHES "${word}")
      message("${f} names ${word}.")
      set(status 1)
    endif()
  endforeach()
endforeach()

if(status)
  message(FATAL_ERROR "Old tools or adapters of Calc-U-59 are left (LIB-11 §11.5, §11.6 H4).")
endif()

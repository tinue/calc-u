# CalcU1600ShotsThroughAttach: the screenshot runner steps the machine
# through harness attach only. Fails if ShotRunner.cpp names one of the ways
# it used to step it itself.
#   cmake -DSOURCE=<ShotRunner.cpp> -P check_shots_through_attach.cmake
if(NOT DEFINED SOURCE)
  message(FATAL_ERROR "SOURCE is not set")
endif()
file(READ "${SOURCE}" text)
set(found "")
foreach(word "controller()" "pacer()" "setFrozen" "runFor" "pasteText")
  string(FIND "${text}" "${word}" at)
  if(NOT at EQUAL -1)
    list(APPEND found "${word}")
  endif()
endforeach()
if(found)
  message(FATAL_ERROR "${SOURCE} steps the machine itself: ${found}")
endif()
message(STATUS "ShotRunner.cpp steps the machine through attach only")

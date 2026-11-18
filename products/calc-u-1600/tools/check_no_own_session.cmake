# The product has no session class of its own, the Qt6 frontend hosts the
# library's app session, and it never sends suspend (it keeps running in the
# background). Run from the product folder: cmake -P tools/check_no_own_session.cmake
# (CTest CalcU1600NoOwnSession, Frequent).

set(failures "")

# No AppSession / MachineSession class (nor the old Pacer) in the app layer.
file(GLOB_RECURSE app_sources app/*.hpp app/*.cpp)
foreach(f IN LISTS app_sources)
  file(STRINGS "${f}" hits REGEX "(class|struct)[ \t]+(AppSession|MachineSession|Pacer)[ \t]*[:{;]")
  if(hits)
    list(APPEND failures "${f}: ${hits}")
  endif()
endforeach()
foreach(old app/AppSession.hpp app/MachineSession.hpp app/Pacer.hpp frontends/qt6/app/SyncOperations.hpp)
  if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../${old}" OR EXISTS "${old}")
    list(APPEND failures "${old} still exists")
  endif()
endforeach()

# The Qt6 frontend hosts the library's app session.
file(GLOB_RECURSE qt_sources frontends/qt6/app/*.hpp frontends/qt6/app/*.cpp frontends/qt6/app/*.mm)
set(hosts FALSE)
foreach(f IN LISTS qt_sources)
  file(STRINGS "${f}" inc REGEX "#include \"sessions/AppSession.hpp\"")
  if(inc)
    set(hosts TRUE)
  endif()
  # ... and never suspends it (no suspend command, no setSuspended).
  file(STRINGS "${f}" sus REGEX "kSuspendCommand|\"suspend\"|setSuspended")
  if(sus)
    list(APPEND failures "${f}: sends suspend: ${sus}")
  endif()
  file(STRINGS "${f}" run REGEX "runNow|runToCompletion")
  if(run)
    list(APPEND failures "${f}: runs an operation to its end: ${run}")
  endif()
endforeach()
if(NOT hosts)
  list(APPEND failures "frontends/qt6 does not include sessions/AppSession.hpp")
endif()

if(failures)
  list(JOIN failures "\n  " text)
  message(FATAL_ERROR "CalcU1600NoOwnSession:\n  ${text}")
endif()
message(STATUS "CalcU1600NoOwnSession: no own session class; Qt6 hosts the library's app session and sends no suspend")

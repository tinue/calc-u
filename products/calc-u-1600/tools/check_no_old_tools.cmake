# LIB-11 Part D, D.6 H4 (M-51; plan M-51 "Green after": no old CLI and no
# adapter of Calc-U-1600 left): today's tools are gone with their files,
# targets and IDE run configurations, the probes' instrumentation
# (PC1600_POWER_PROBE) is out of core/, and the adapters' header
# core/Machine/LegacyCalls.hpp is gone. Their functions are `calcu`
# commands (LIB-11 AC-18). Run from products/calc-u-1600 with
# `cmake -P tools/check_no_old_tools.cmake` (CTest CalcU1600NoOldTools).

set(gone
  tools/pc1500_cli.cpp tools/pc1600_cli.cpp tools/CliCommon.hpp tools/Ce158CliPeer.hpp tools/CliPresetRun.hpp
  tools/build_cli.sh tools/build_pc1600_cli.sh tools/build_tool.sh
  tools/pc1600_plotter_probe.cpp tools/pc1600_power_probe.cpp tools/pc1600_uart_probe.cpp
  tools/build_pc1600_plotter_probe.sh tools/build_pc1600_power_probe.sh tools/build_pc1600_uart_probe.sh
  core/PC1600/PC1600PowerProbe.hpp core/Machine/LegacyCalls.hpp core/Machine/LegacyCalls.cpp
  tests/cli_rom_tests.sh dev/tape-matrix dev/loader-matrix
  ../../.run/pc1500_cli.run.xml ../../.run/pc1600_cli.run.xml ../../.run/pc1600_plotter_probe.run.xml
  ../../.run/pc1600_power_probe.run.xml ../../.run/pc1600_uart_probe.run.xml)
set(status 0)
foreach(f ${gone})
  if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../${f})
    message("${f} is still there (M-51 removed it).")
    set(status 1)
  endif()
endforeach()

# No target of the old tools in the product's CMake file.
file(READ ${CMAKE_CURRENT_LIST_DIR}/../CMakeLists.txt cmake_text)
foreach(target pc1500_cli pc1600_cli pc1600_plotter_probe pc1600_power_probe pc1600_uart_probe calcu_core_probe)
  if(cmake_text MATCHES "add_(executable|library)\\(${target}[ \n]" OR cmake_text MATCHES "calcu_add_core\\(${target}\\)")
    message("CMakeLists.txt still builds ${target}.")
    set(status 1)
  endif()
endforeach()

# No probe instrumentation and no adapter header in the sources.
file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/../core/*.cpp ${CMAKE_CURRENT_LIST_DIR}/../core/*.hpp
                          ${CMAKE_CURRENT_LIST_DIR}/../app/*.cpp ${CMAKE_CURRENT_LIST_DIR}/../app/*.hpp
                          ${CMAKE_CURRENT_LIST_DIR}/../tests/*.cpp ${CMAKE_CURRENT_LIST_DIR}/../tests/*.hpp)
foreach(f ${sources})
  file(READ ${f} text)
  if(text MATCHES "PC1600_POWER_PROBE")
    message("${f} names PC1600_POWER_PROBE.")
    set(status 1)
  endif()
  if(text MATCHES "#include[^\n]*LegacyCalls\\.hpp")
    message("${f} includes LegacyCalls.hpp.")
    set(status 1)
  endif()
endforeach()

if(status)
  message(FATAL_ERROR "Old tools or adapters of Calc-U-1600 are left (LIB-11 Part D, D.5, D.7).")
endif()

# `calcu` with Calc-U-59 runs, as a
# process of its own, in the form of library/harness/tests/cli/cli_cases.cmake:
#   cmake -DCALCU=<the calcu program> -P tests/calcu_runs.cmake
# from products/calc-u-59 (the ROMs in roms/). The CTest CalcU59CalcuRuns
# (Frequent) and every desktop job of .github/workflows/calc-u-59.yml run it.
# Case calcu_runs_calc_u_59: `wait 1s then display-text`, exit status 0 and
# the display of the TI-59 at key readiness.

if(NOT CALCU)
  message(FATAL_ERROR "calcu_runs_calc_u_59: -DCALCU=<program> is needed")
endif()
execute_process(COMMAND ${CALCU} --product c59 wait 1s then display-text
                OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE status)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "calcu_runs_calc_u_59: status ${status}: ${err}")
endif()
string(REPLACE "\r\n" "\n" out "${out}")
if(NOT out MATCHES "^done 2\nticks: [0-9]+\nseconds: 1\\.[0-9]+\ndone 2\n          0 \nindicators:")
  message(FATAL_ERROR "calcu_runs_calc_u_59: replied: ${out}")
endif()
message(STATUS "calcu_runs_calc_u_59: ok")

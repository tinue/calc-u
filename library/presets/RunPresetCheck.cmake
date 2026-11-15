# The command of calcu_add_preset_check() (PresetChecks.cmake): cmake
# -DCALCU=<program> -DPRODUCT=<word> -DFILES=<a|b|…> [-DFILL=<script>]
# -DWORK=<folder> -P RunPresetCheck.cmake. Fails with the program's reply
# when a file fails.
string(REPLACE "|" ";" files "${FILES}")
set(checked "")
if(FILL)
  file(REMOVE_RECURSE ${WORK})
  file(MAKE_DIRECTORY ${WORK})
  foreach(f ${files})
    get_filename_component(fname ${f} NAME)
    string(REGEX REPLACE "\\.in$" "" fname "${fname}")
    execute_process(COMMAND ${CMAKE_COMMAND} -DIN=${f} -DOUT=${WORK}/${fname} -P ${FILL} RESULT_VARIABLE status)
    if(NOT status EQUAL 0)
      message(FATAL_ERROR "The fill script failed on ${f}.")
    endif()
    list(APPEND checked ${WORK}/${fname})
  endforeach()
else()
  set(checked ${files})
endif()
execute_process(COMMAND ${CALCU} --product ${PRODUCT} check-preset ${checked}
                OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE status)
message("${out}${err}")
if(NOT status EQUAL 0)
  message(FATAL_ERROR "check-preset failed (status ${status}).")
endif()

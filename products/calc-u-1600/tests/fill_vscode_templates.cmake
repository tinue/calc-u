# The VS Code templates filled in as the extension fills them: every `{{name}}` becomes the target's
# default name (vscode/calcu1600-debug/scaffold.js: `rom` for a bus ROM
# template, `main` otherwise; the rule of scaffold.js:58). The file a filled
# preset reads when it is checked (the bus ROM, the program) is written
# beside it as a small stand-in, as a build would.
#   cmake -DIN=<template> -DOUT=<file> -P fill_vscode_templates.cmake
get_filename_component(dir ${IN} DIRECTORY)
get_filename_component(target ${dir} NAME)
if(target MATCHES "bus-rom")
  set(name rom)
else()
  set(name main)
endif()
file(READ ${IN} text)
string(REPLACE "{{name}}" "${name}" text "${text}")
file(WRITE ${OUT} "${text}")
get_filename_component(outdir ${OUT} DIRECTORY)
# 16 bytes standing in for the build (a bus ROM must not be empty).
file(WRITE ${outdir}/${name}.bin "0123456789abcdef")

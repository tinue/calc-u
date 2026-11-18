#!/bin/sh
# The Qt6, Web and Swift frontends
# draw the model, the module, the printer and the card controls from the
# control state; no file of theirs reads the model
# facts or the interim topics: hasCardReader, cu59::info(, c59.model,
# c59.web.model, allModules, MachineModel (the Swift copy of the model facts).
# Run from products/calc-u-59. Exit 1 with the lines found.
set -u
found=$(grep -rnE 'hasCardReader|cu59::info\(|c59\.model|c59\.web\.model|allModules|MachineModel' \
    frontends/qt6/app frontends/web/src frontends/web/bindings.cpp frontends/swift/App frontends/swift/Bridge 2>/dev/null \
    | grep -vE '^[^:]+:[0-9]+:\s*(//|\*|///)')
[ -e frontends/swift/App/MachineModel.swift ] && found="$found
frontends/swift/App/MachineModel.swift exists"
if [ -n "$found" ]; then
    echo "A frontend decides from the model facts or an interim topic:"
    echo "$found"
    exit 1
fi
exit 0

#!/bin/sh
# The Qt6, Web and Swift frontends draw the model, the module, the printer
# and the card controls from the control state; no file of theirs reads the
# app layer's model facts (MachineModelInfo, hasCardReader, cu59::info).
# Run from products/calc-u-59. Exit 1 with the lines found.
set -u
found=$(grep -rnE 'MachineModelInfo|hasCardReader|cu59::info\(' \
    frontends/qt6/app frontends/web/src frontends/web/bindings.cpp frontends/swift/App frontends/swift/Bridge 2>/dev/null \
    | grep -vE '^[^:]+:[0-9]+:\s*(//|\*|///)')
if [ -n "$found" ]; then
    echo "A frontend decides from the model facts:"
    echo "$found"
    exit 1
fi
exit 0

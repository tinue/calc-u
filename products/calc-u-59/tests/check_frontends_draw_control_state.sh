#!/bin/sh
# LIB-4 AC-15 (design §10.6 T59-6, D-19): the Qt6 and Web frontends draw the
# model, the module, the printer and the card controls from the control
# state (M-57); no file of theirs reads the model facts or the interim
# topics: hasCardReader, cu59::info(, c59.model, c59.web.model, allModules.
# The Swift frontend reads `c59.model` until C59-8 (M-58), which adds it
# here with the check that App/MachineModel.swift is gone.
# Run from products/calc-u-59. Exit 1 with the lines found.
set -u
found=$(grep -rnE 'hasCardReader|cu59::info\(|c59\.model|c59\.web\.model|allModules' \
    frontends/qt6/app frontends/web/src frontends/web/bindings.cpp 2>/dev/null \
    | grep -vE '^[^:]+:[0-9]+:\s*(//|\*|///)')
if [ -n "$found" ]; then
    echo "A frontend decides from the model facts or an interim topic (LIB-4 AC-15):"
    echo "$found"
    exit 1
fi
exit 0

#!/bin/sh
# C59-1 AC-17 (design T12): the app layer reaches the machine's model,
# advances and tick count, operations, keys and display through the machine
# interface only. Outside core/ and tests/ no file includes a core-internal
# header, and the adapters (core/TIAdapters.hpp) declare none of the old
# machine calls. Run from products/calc-u-59 (CTest CalcU59CoreSeam).
set -eu
cd "$(dirname "$0")/.."
status=0

internal='(TI59Machine|TMC0501|MachineVariant|RAM|ROM|TIMachine|TIKeys|TIRomRanges|TIMachineTesting)\.hpp'
hits=$(grep -rnE "#include *[<\"]([./]*core/|calcu59/core/)$internal" app frontends tools \
         --include='*.cpp' --include='*.hpp' --include='*.h' --include='*.mm' --include='*.m' 2>/dev/null \
       | grep -vE '/(build|dist|node_modules|DerivedData)/' || true)
if [ -n "$hits" ]; then
  echo "core-internal header included outside core/ and tests/:"
  echo "$hits"
  status=1
fi

# Declarations in TIAdapters.hpp, comments stripped: none of the machine's own
# calls; step/stepN only in TIDebugTargetAdapter.
bad=$(sed -e 's://.*$::' core/TIAdapters.hpp | awk '
  /^class [A-Za-z]+/ { cls = $2 }
  /^};/ { cls = "" }
  /(^|[^A-Za-z_])(pressKey|releaseKey|takeDisplay|reset|elapsedTicks|variant)[ ]*\(/ { print FILENAME ":" NR ": " $0 }
  cls != "TIDebugTargetAdapter" && /(^|[^A-Za-z_])(step|stepN)[ ]*\(/ { print "core/TIAdapters.hpp:" NR ": " $0 }
')
if [ -n "$bad" ]; then
  echo "core/TIAdapters.hpp declares a call that goes through the machine interface:"
  echo "$bad"
  status=1
fi

[ $status -eq 0 ] && echo "core seam: ok"
exit $status

#!/bin/sh
# Every frontend of a product hosts the library's app session: the product
# defines no AppSession, MachineSession or Pacer of its own, the Qt6 window
# goes through calcu::qt6::SessionHost and the Web page through
# calcu::shell::HostSession, and the Qt6 frontend sends neither suspend nor
# continue (the desktop app keeps running in the background; Swift and the
# Web pause).
#
# Usage: check_no_own_session.sh <product-dir>
# CTest <Product>NoOwnSession (calcu_product_source_checks).
set -uf  # -f: the --include patterns stay unexpanded
[ $# -eq 1 ] || { echo "usage: $0 <product-dir>" >&2; exit 2; }
cd "$1" || exit 2
fail=0
src="--include=*.cpp --include=*.hpp --include=*.h --include=*.mm"
not_comment='^[^:]+:[0-9]+:[[:space:]]*(//|\*|///)'

dirs=""
for d in app frontends; do [ -d "$d" ] && dirs="$dirs $d"; done
# shellcheck disable=SC2086
own=$(grep -rnE $src '(class|struct)[[:space:]]+(AppSession|MachineSession|Pacer)[[:space:]]*[:{;]' $dirs 2>/dev/null \
      | grep -vE "$not_comment")
if [ -n "$own" ]; then
    echo "The product defines a session class of its own:"
    echo "$own"
    fail=1
fi

if [ -d frontends/qt6/app ]; then
    # shellcheck disable=SC2086
    if ! grep -rqE $src '#include "qt6/SessionHost\.hpp"' frontends/qt6/app; then
        echo "frontends/qt6/app does not include qt6/SessionHost.hpp"
        fail=1
    fi
    # shellcheck disable=SC2086
    sus=$(grep -rnE $src 'kSuspendCommand|kContinueCommand|setSuspended|"suspend"|"continue"' frontends/qt6/app \
          | grep -vE "$not_comment")
    if [ -n "$sus" ]; then
        echo "The Qt6 frontend sends suspend or continue:"
        echo "$sus"
        fail=1
    fi
fi

if [ -f frontends/web/bindings.cpp ] && ! grep -qE '#include "shell/HostSession\.hpp"' frontends/web/bindings.cpp; then
    echo "frontends/web/bindings.cpp does not include shell/HostSession.hpp"
    fail=1
fi

[ $fail -eq 0 ] && echo "no own session: ok"
exit $fail

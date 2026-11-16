#!/bin/sh
# C1600-3 EC-1, D-6 (design T13): the harness with `serve --dap` and a port
# outside 1024 to 65535 says so and ends with exit status 2; the server does
# not listen. Run from the product directory with CALCU set to the harness.
set -u
out=$("$CALCU" --product 1600 serve --dap 80 </dev/null 2>&1)
status=$?
if [ "$status" -ne 2 ]; then
  echo "exit status $status, not 2: $out"
  exit 1
fi
case "$out" in
  *"The DAP port must be from 1024 to 65535."*) ;;
  *) echo "no reason given: $out"; exit 1 ;;
esac
# --dap belongs to serve.
out=$("$CALCU" --product 1600 --dap 32168 ticks </dev/null 2>&1)
status=$?
if [ "$status" -ne 2 ]; then
  echo "one-shot with --dap: exit status $status, not 2: $out"
  exit 1
fi
echo "ok"

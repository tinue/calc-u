#!/bin/sh
# LIB-10 AC-42 (design §10.10, D16-8; the CTest CalcU1600ReadTrace): the
# harness writes a short PC-1600 trace through the library's trace session,
# and tools/read_trace.py -- a script, unchanged -- reads it: both CPUs'
# records, the session's start and end. Run from products/calc-u-1600 with
# CALCU set to the harness program.
set -eu
: "${CALCU:?CALCU must name the calcu program}"
out=$(mktemp -d "${TMPDIR:-/tmp}/calcu1600-read-trace.XXXXXX")
trap 'rm -rf "$out"' EXIT
"$CALCU" --product 1600 --output-folder "$out" model PC-1600 then trace t.bin then wait 300ms then trace off >/dev/null
python3 tools/read_trace.py "$out/t.bin" > "$out/t.txt"
grep -q "SESSION START" "$out/t.txt" || { echo "no SESSION START in the trace" >&2; exit 1; }
grep -q "SESSION END" "$out/t.txt" || { echo "no SESSION END in the trace" >&2; exit 1; }
echo "read_trace.py read $(grep -c . "$out/t.txt") lines of a PC-1600 trace"

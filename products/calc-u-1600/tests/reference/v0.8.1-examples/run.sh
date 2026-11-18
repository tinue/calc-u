#!/bin/bash
# Reference runs of the 32 Calc-U-1600 examples with the
# CLIs of v0.8.1 (= b4ea8c8 + docs). Each run in a fresh copy of examples/.
set -u
SRC=$1; OUT=$2
cd "$SRC"
for p in $(cd examples && find . \( -name "*.pc15*" -o -name "*.pc16*" \) | sort); do
  p=${p#./}; name=$(echo "$p" | tr '/' '_')
  work=$(mktemp -d); cp -R examples "$work/examples"; mkdir -p "$work/save"
  case "$p" in *.pc1600) cli=headless/pc1600_cli; extra="--dump-basic --inspect programs";;
               *) cli=headless/pc1500_cli; extra="--inspect basic";; esac
  start=$(date +%s)
  timeout 900 $cli --preset "$work/examples/$p" --save-dir "$work/save" --lcd-text - \
     --inspect pointers $extra --inspect dump-basic --inspect dump-ml \
     > "$OUT/$name.out" 2> "$OUT/$name.log"
  rc=$?
  end=$(date +%s)
  { echo "preset: examples/$p"; echo "cli: $cli"; echo "exit: $rc"; echo "seconds: $((end-start))";
    echo "--- files changed in the copy of examples/:"; diff -rq examples "$work/examples" ;
    echo "--- saved media:"; ls -la "$work/save" | tail -n +4; } > "$OUT/$name.meta"
  rm -rf "$work"
  echo "$p rc=$rc $((end-start))s"
done

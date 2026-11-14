#!/usr/bin/env bash
# Self-test of library/settings/check_platform_stores.sh (LIB-9 design §6,
# T-14; CTest LibraryPlatformStoreCheck): on a tree it builds itself, the
# check passes on clean sources and on allowed uses, and fails on every
# store it looks for.
set -euo pipefail

check="$(cd "$(dirname "$0")/.." && pwd)/check_platform_stores.sh"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
fail=0

expect() {  # expect <0|1> <description> <args...>
  local want="$1" what="$2"
  shift 2
  local got=0
  bash "$check" "$@" >"$work/out" 2>&1 || got=$?
  if [ "$got" -ne "$want" ]; then
    echo "FAIL $what: exit $got, wanted $want"
    cat "$work/out"
    fail=1
  else
    echo "ok   $what"
  fi
}

mkdir -p "$work/clean/qt6" "$work/clean/web/node_modules/x"
echo 'int main() { return 0; }' > "$work/clean/qt6/main.cpp"
echo 'localStorage.setItem("x", 1);' > "$work/clean/web/node_modules/x/lib.js"  # outside the check
expect 0 "clean tree" "$work/clean"

for store in 'QSettings s("a", "b");' 'UserDefaults.standard.set(1, forKey: "k")' \
             '@AppStorage("k") var k = 1' 'localStorage.getItem("calcu/k")' \
             '[[NSUserDefaults standardUserDefaults] registerDefaults:@{}];'; do
  rm -rf "$work/dirty" && mkdir -p "$work/dirty/app"
  echo "$store" > "$work/dirty/app/File.swift"
  expect 1 "finds: $store" "$work/dirty"
done

# An allowed use passes; the same text in another file, or another use in
# the allowed file, does not.
rm -rf "$work/allowed" && mkdir -p "$work/allowed/swift" "$work/allowed/web"
echo 'UserDefaults.standard.data(forKey: "traceCustomPathBookmark")' > "$work/allowed/swift/AppSettings.swift"
printf '# the bookmark is a grant, not a setting (AC-34)\nswift/AppSettings.swift: traceCustomPathBookmark\n' \
  > "$work/allow.txt"
expect 0 "allowed use" "$work/allowed" "$work/allow.txt"
echo 'UserDefaults.standard.set(1, forKey: "ledFontStyle")' >> "$work/allowed/swift/AppSettings.swift"
expect 1 "other use in the allowed file" "$work/allowed" "$work/allow.txt"
echo 'localStorage.getItem("traceCustomPathBookmark")' > "$work/allowed/web/engine.js"
sed -i.bak '$d' "$work/allowed/swift/AppSettings.swift" && rm -f "$work/allowed/swift/AppSettings.swift.bak"
expect 1 "allowed text in another file" "$work/allowed" "$work/allow.txt"

expect 2 "no tree" "$work/none"

exit "$fail"

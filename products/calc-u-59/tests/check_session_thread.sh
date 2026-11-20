#!/bin/sh
# The app layer runs on one thread: no std::thread in app/ (the Swift
# frontend's session thread is the library's shell/SessionThread), and the
# Swift bridge drives the session only through it (it calls no tick itself).
# Run from products/calc-u-59 (CTest CalcU59SessionThread). Exit 1 with the lines found.
set -u
fail=0
threads=$(grep -rnE 'std::thread\b' app | grep -vE '^[^:]+:[0-9]+:\s*//')
if [ -n "$threads" ]; then
    echo "A thread in the app layer (the library's SessionThread hosts the session):"
    echo "$threads"
    fail=1
fi
ticks=$(grep -nE '(->|\.)tick\(' frontends/swift/Bridge/* 2>/dev/null)
if [ -n "$ticks" ]; then
    echo "The Swift bridge reaches past the session thread:"
    echo "$ticks"
    fail=1
fi
[ $fail -eq 0 ] && echo "ok"
exit $fail

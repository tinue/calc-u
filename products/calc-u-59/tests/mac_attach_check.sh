#!/bin/sh
# The Debug Mac app with harness attach (C59-5 S6: AC-8, AC-9, AC-10; LIB-17
# AC-19 for Swift macOS, design P59-5). Checkpoint (CTest CalcU59MacAttach):
# it shows the app and, for 30 s, a grey window over every screen.
#
#   tests/mac_attach_check.sh <calcu> [<app>]
#
# <calcu>: the harness (build/tests/library/harness/cli/calcu). <app>: the
# Debug Calc-U-59.app (else built with xcodebuild into build/xcode). The app
# runs with --UITesting (its settings in memory) on port 59171.
#   1. `calcu attach` works (AC-10): `ticks` is answered.
#   2. AC-19: one minute of wall time, emulated time grows by one minute
#      within AC-1's bound (the TI-59's 20 ms slice), one host call (a
#      reading is made between two calls) and half of each round trip.
#   3. AC-9: the window covered for 30 s: emulated time keeps wall time,
#      within the same bound.
#   4. AC-8 (D-10): hidden for 5 s, then shown: whether a command is refused
#      while hidden (macOS reported the background) is recorded; after it is
#      shown, emulated time passes again.
set -u
CALCU="$1"
HERE="$(cd "$(dirname "$0")" && pwd)"
PRODUCT="$(cd "$HERE/.." && pwd)"
REPO="$(cd "$PRODUCT/../.." && pwd)"
APP="${2:-}"
if [ -z "$APP" ]; then
  xcodebuild build -project "$PRODUCT/frontends/swift/Calc-U-59.xcodeproj" -scheme Calc-U-59 -configuration Debug \
    -destination platform=macOS -derivedDataPath "$REPO/build/xcode" -quiet || exit 1
  APP="$REPO/build/xcode/Build/Products/Debug/Calc-U-59.app"
fi
PYTHON="python3"
command -v uv >/dev/null && PYTHON="uv run --no-project python"
exec $PYTHON -I - "$CALCU" "$APP" "$HERE/mac_window.swift" <<'PY'
import subprocess, sys, time

calcu, app, window = sys.argv[1], sys.argv[2], sys.argv[3]
PORT = "59171"
BOUND = 0.020 + 1 / 60
failures = []

def check(ok, what):
    print(("ok: " if ok else "FAIL: ") + what, flush=True)
    if not ok:
        failures.append(what)

def attach(*commands):
    sent = time.time()
    r = subprocess.run([calcu, "attach", "--port", PORT], input="\n".join(commands + ("quit",)) + "\n",
                       capture_output=True, text=True, timeout=30)
    back = time.time()
    return r.stdout.splitlines(), (sent + back) / 2, back - sent

def reading():
    lines, wall, rtt = attach("ticks")
    if not lines or lines[0] != "done 2":
        return None, wall, rtt, lines
    return float(lines[2].split()[1]), wall, rtt, lines

def keeps(seconds, what):
    a = reading()
    time.sleep(seconds)
    b = reading()
    if a[0] is None or b[0] is None:
        check(False, f"{what}: no emulated time ({a[3]}, {b[3]})")
        return
    emulated, wall = b[0] - a[0], b[1] - a[1]
    bound = BOUND + (a[2] + b[2]) / 2
    check(abs(emulated - wall) <= bound,
          f"{what}: wall {wall:.4f} s, emulated {emulated:.4f} s, difference {emulated - wall:+.4f} s, bound {bound:.4f} s")

# Through Launch Services, as a user starts it: started directly from a shell
# the app is not a regular foreground app and falls seconds behind a minute.
subprocess.run(["open", "-n", app, "--args", "--UITesting", "--attach-port", PORT], check=True)
pid = None
for _ in range(40):
    r = subprocess.run(["pgrep", "-n", "-f", f"Calc-U-59.app/Contents/MacOS/Calc-U-59 --UITesting --attach-port {PORT}"],
                       capture_output=True, text=True)
    if r.stdout.strip():
        pid = int(r.stdout.split()[0])
        break
    time.sleep(0.25)
if pid is None:
    sys.exit("FAIL: the app did not start")
try:
    lines = []
    for _ in range(60):
        lines, _, _ = attach("ticks")
        if lines and lines[0] == "done 2":
            break
        time.sleep(0.5)
    check(bool(lines) and lines[0] == "done 2", f"calcu attach --port {PORT}: ticks answered ({lines[:1]})")
    time.sleep(1)
    keeps(60, "AC-19, one minute in front")

    cover = subprocess.Popen(["swift", window, "cover", "34"])
    time.sleep(3)   # the cover is up
    keeps(30, "AC-9, the window covered for 30 s")
    cover.wait()

    subprocess.run(["swift", window, "hide", str(pid)], check=False)
    time.sleep(2)
    hidden, _, _ = attach("ticks")
    print(f"note: a command while hidden: {hidden[:1]} "
          f"({'macOS reported the background' if hidden[:1] == ['refused: the app is suspended'] else 'no background reported'})")
    time.sleep(3)
    subprocess.run(["swift", window, "unhide", str(pid)], check=False)
    time.sleep(2)
    keeps(2, "AC-8, shown again: emulated time passes")
finally:
    subprocess.run(["kill", str(pid)], check=False)
print("FAILED: " + "; ".join(failures) if failures else "all passed")
sys.exit(1 if failures else 0)
PY

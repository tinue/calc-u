#!/usr/bin/env python3
"""The harness without a ROM file: `calcu --product c59` on a ROM folder without one file of the full
set (CALCU59_ROMS_DIR) ends with exit status 2 and an error that names it,
and on the full set starts as before. Run from products/calc-u-59 with
CALCU set to the program (CTest CalcU59HarnessRomMissing, Frequent)."""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PRODUCT = Path(__file__).resolve().parent.parent
CALCU = os.environ.get("CALCU") or str(PRODUCT.parent.parent / "build" / "tests" / "library" / "harness" / "cli" / "calcu")


def run(roms: Path) -> subprocess.CompletedProcess:
    env = dict(os.environ, CALCU59_ROMS_DIR=str(roms))
    return subprocess.run([CALCU, "--product", "c59", "key", "5", "+", "7", "=", "then", "wait", "500ms", "then", "display-text"],
                          env=env, capture_output=True, text=True, timeout=120)


def main() -> int:
    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        roms = Path(tmp) / "roms"
        roms.mkdir()
        for f in PRODUCT.glob("roms/*"):
            if f.is_file() and (f.suffix == ".txt" or f.name.endswith(".rom.yaml")):
                shutil.copy(f, roms / f.name)
        ok = run(roms)
        if ok.returncode != 0 or "12" not in ok.stdout:
            failures.append(f"full set: exit {ok.returncode}, {ok.stdout.strip()} {ok.stderr.strip()}")
        for name in ("TMC0583.txt", "RP.rom.yaml"):
            (roms / name).rename(Path(tmp) / name)
            r = run(roms)
            if r.returncode != 2:
                failures.append(f"{name} missing: exit {r.returncode}, not 2")
            if f"{name}: missing" not in r.stderr:
                failures.append(f"{name} missing: the error does not name it: {r.stderr.strip()}")
            (Path(tmp) / name).rename(roms / name)
    for f in failures:
        print("FAIL", f)
    print("harness ROM lookup: ok" if not failures else f"{len(failures)} failures")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

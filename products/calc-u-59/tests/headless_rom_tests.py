#!/usr/bin/env python3
"""LIB-12 AC-7, AC-10 for tools/headless (design §6, P5): ti59.py on a ROM
folder without one file of the full set ends with an error that names it,
and on the full set starts as before. Run from products/calc-u-59 with
TI59HEADLESS_LIB set (CTest CalcU59HeadlessRomMissing, Frequent)."""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PRODUCT = Path(__file__).resolve().parent.parent
TI59 = PRODUCT / "tools" / "headless" / "ti59.py"


def run(roms: Path) -> subprocess.CompletedProcess:
    env = dict(os.environ, TI59_ROMS_DIR=str(roms))
    return subprocess.run([sys.executable, str(TI59), "5", "+", "7", "=", "--summary"],
                          env=env, capture_output=True, text=True, timeout=120)


def main() -> int:
    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        roms = Path(tmp) / "roms"
        shutil.copytree(PRODUCT / "roms", roms)
        ok = run(roms)
        if ok.returncode != 0 or "12" not in ok.stdout:
            failures.append(f"full set: exit {ok.returncode}, {ok.stderr.strip()}")
        for name in ("TMC0583.txt", "RP.rom.yaml"):
            (roms / name).rename(Path(tmp) / name)
            r = run(roms)
            if r.returncode != 1:
                failures.append(f"{name} missing: exit {r.returncode}, not 1")
            if f"{name}: missing" not in r.stderr or "The full set of ROM files is needed" not in r.stderr:
                failures.append(f"{name} missing: the error does not name it: {r.stderr.strip()}")
            (Path(tmp) / name).rename(roms / name)
    for f in failures:
        print("FAIL", f)
    print("headless ROM lookup:", "ok" if not failures else f"{len(failures)} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

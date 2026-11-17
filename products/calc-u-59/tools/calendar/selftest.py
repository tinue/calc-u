"""The check of C59-2 AC-18 (design §6 T12; CTest CalcU59CalendarScripts,
Frequent): fastcal.py writes calendar-09 equal to the tracked
examples/calendar/calendar-09-dispatch.ti59, and years.py finds the year in
it, runs it through calcu for 2026 and its rows check (and equal Acosta's).

CLI: selftest.py [--calcu PATH]   (run from products/calc-u-59 or anywhere)"""
import os
import subprocess
import sys
import tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from calcu59 import ROOT, calcu_path  # noqa: E402

args = sys.argv[1:]
program = calcu_path(args)
tracked = os.path.join(ROOT, 'examples', 'calendar', 'calendar-09-dispatch.ti59')
with tempfile.TemporaryDirectory() as tmp:
    out = os.path.join(tmp, 'calendar-09-dispatch.ti59')
    subprocess.run([sys.executable, os.path.join(HERE, 'fastcal.py'), out], check=True, stdout=subprocess.DEVNULL)
    if open(out, encoding='utf-8').read() != open(tracked, encoding='utf-8').read():
        print('fastcal.py does not write the tracked', tracked)
        sys.exit(1)
print('fastcal.py writes', os.path.relpath(tracked, ROOT))
r = subprocess.run([sys.executable, os.path.join(HERE, 'years.py'), tracked, '2026', '--calcu', program])
sys.exit(r.returncode)

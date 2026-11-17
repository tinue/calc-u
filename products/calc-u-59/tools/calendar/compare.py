"""Time calendar programs, each with its own keys, and check them.

CLI: compare.py file.ti59... [--calcu PATH]
  For each preset: steps (without trailing zeros), seconds from the last key
  to the last printed line (`c59 line-times`, tools/calcu59.py), the year found in
  the printout, and the day-row check against Python's calendar module.
  zsh note: if `uv` is aliased with noglob, expand globs with $(ls ...)."""
import os
import re
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE)); sys.path.insert(0, HERE)
from calcu59 import Run, calcu_path, line_times, split_preset  # noqa: E402
from check import check  # noqa: E402


def steps_of(path, program):
    """The program's keycodes as the preset writes them, without trailing zeros."""
    run = Run(program)
    try:
        run.preset(split_preset(path)[0], os.path.basename(path))
        prog = run.program()
    finally:
        run.close()
    while prog and prog[-1] == 0:
        prog.pop()
    return prog


if __name__ == '__main__':
    args = sys.argv[1:]
    program = calcu_path(args)
    for path in args:
        out = line_times(path, program=program)
        lines = [l for _, l in out]
        years = re.findall(r'(\d{4})\.', ' '.join(lines))
        y = int(years[0]) if years else 2026
        prog = steps_of(path, program)
        got, exp, bad = check(lines, y)
        print(f'{path:45} {len(prog):4} steps  {out[-1][0]:7.1f} s  year {y}  rows {got}/{exp}  bad={bad[:1]}')

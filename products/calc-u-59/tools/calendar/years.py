"""Run a calendar program for several years and check each printout.

CLI: years.py file.ti59 year... [--no-acosta] [--write FOLDER] [--calcu PATH]
  For each year: replaces the four digit steps before the first `- key:`
  step that names A (matrix code 11 in 2.3.0's KEYSTROKES) in the preset's
  keys, runs it through calcu (tools/calcu59.py), checks the day rows
  against Python's calendar module and compares the printout line for line
  with Acosta's program (examples/calendar/calendar-08-acosta.ti59) run for
  the same year (--no-acosta: not). The printed time is from the last key
  until the display lights up again (`ti59 run-time`). --write FOLDER
  writes a copy of the preset for each year, with that year's digits.
  Exit status 1 when a year's rows do not check.
Library: with_year(path, year) -> the steps for calcu59.run_preset."""
import os
import re
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE)); sys.path.insert(0, HERE)
from calcu59 import ROOT, calcu_path, run_preset, split_preset  # noqa: E402
from check import check  # noqa: E402

ACOSTA = os.path.join(ROOT, 'examples', 'calendar', 'calendar-08-acosta.ti59')


def year_steps(steps):
    """The indexes of the four digit `key` steps before the first one that names A."""
    keys = [i for i, (verb, _) in enumerate(steps) if verb == 'key']
    a = next(n for n, i in enumerate(keys) if 'A' in steps[i][1].split())
    found = keys[a - 4:a]
    assert all(re.fullmatch(r'\d', steps[i][1]) for i in found), 'no four digit steps before A'
    return found


def with_year(path, year):
    """The preset's steps with the year's digits before the first A."""
    steps = split_preset(path)[1]
    for i, c in zip(year_steps(steps), '%04d' % year):
        steps[i] = ('key', c)
    return steps


def write_year(path, year, folder):
    """A copy of the preset whose digit steps before A key `year`."""
    lines = open(path, encoding='utf-8').read().split('\n')
    at = [i for i, l in enumerate(lines) if re.match(r'^\s*-\s*key:\s*', l)]
    steps = [(m.group(1), m.group(2)) for m in (re.match(r'^\s*-\s*([a-z-]+):\s*(.*?)\s*(#.*)?$', lines[i]) for i in at)]
    for n, c in zip(year_steps(steps), '%04d' % year):
        lines[at[n]] = re.sub(r'(-\s*key:\s*)\d(\s*#\s*)\d', lambda m: m.group(1) + c + m.group(2) + c, lines[at[n]])
    os.makedirs(folder, exist_ok=True)
    out = os.path.join(folder, os.path.basename(path).replace('.ti59', f'-{year}.ti59'))
    open(out, 'w', encoding='utf-8').write('\n'.join(lines))
    return out


if __name__ == '__main__':
    args = sys.argv[1:]
    program = calcu_path(args)
    acosta = '--no-acosta' not in args
    args = [a for a in args if a != '--no-acosta']
    folder = None
    if '--write' in args:
        i = args.index('--write'); folder = args[i + 1]; del args[i:i + 2]
    path, failed = args[0], False
    for y in map(int, args[1:]):
        t, lines, ok = run_preset(path, with_year(path, y), program)
        got, exp, bad = check(lines, y)
        failed = failed or got != exp or bool(bad) or not ok
        same = ''
        if acosta:
            _, ref, _ = run_preset(ACOSTA, with_year(ACOSTA, y), program)
            same = f'  same as Acosta: {lines == ref}'
            failed = failed or lines != ref
        print(f'{y}: {t:6.1f} s  finished={ok}  rows {got}/{exp}  bad={bad}{same}')
        if folder:
            print('  wrote', write_year(path, y, folder))
    sys.exit(1 if failed else 0)

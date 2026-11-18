"""Calc-U-59 through the harness `calcu`, for the scripts that stay:
tools/coverage.py, tools/fastmode_matrix.py and the calendar scripts in tools/calendar/. One
`calcu --product c59 serve` per Run, in emulated time only; reference/Harness.md
has the commands and the recipes.

The calcu program: --calcu PATH on a script's command line, else the
library's finder (calcu_serve.find_calcu: the environment variable CALCU,
else the repository's build/tests, build/all or build/tools tree), else
build/tools configured (Release, no Qt) and built on first use. Scratch
files go to tools/build/ (ignored).

Timing: `c59 run-time` and `c59 line-times` time on the
calculator's own clock: a key's run time from its press until the display
is lit again and no program runs (memory run_time_measurement); line times
from the command, after the last key is up."""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                               # products/calc-u-59
REPO = os.path.dirname(os.path.dirname(ROOT))
BUILD = os.path.join(HERE, 'build')
sys.path.insert(0, os.path.join(REPO, 'library', 'harness', 'python'))
from calcu_serve import Calcu, find_calcu  # noqa: E402


def calcu_path(argv=None):
    """The calcu program: --calcu PATH in argv (removed from it), find_calcu(), or build/tools built."""
    if argv is not None and '--calcu' in argv:
        i = argv.index('--calcu')
        path = argv[i + 1]
        del argv[i:i + 2]
        return path
    found = find_calcu()
    if found:
        return found
    tree = os.path.join(REPO, 'build', 'tools')
    if not os.path.exists(os.path.join(tree, 'CMakeCache.txt')):
        subprocess.run(['cmake', '-S', REPO, '-B', tree, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
                        '-DCALCU_FRONTEND_QT=OFF', '-DCALCU_PRODUCT_1600=OFF'], check=True, stdout=subprocess.DEVNULL)
    subprocess.run(['cmake', '--build', tree, '--target', 'calcu'], check=True, stdout=subprocess.DEVNULL)
    return os.path.join(tree, 'library', 'harness', 'cli', 'calcu')


def split_preset(path):
    """(the preset's text without its `keys:` sections, the steps of those
    sections in order as (verb, value)). The scripts load the text and press
    the keys themselves, so that they can time them."""
    text = open(path, encoding='utf-8').read().split('\n')
    kept, steps, in_keys = [], [], False
    for line in text:
        top = line[:1] not in (' ', '') and not line.startswith('#')
        if top:
            in_keys = line.split('#')[0].strip() == 'keys:'
            if in_keys:
                continue
        if in_keys:
            m = re.match(r'^\s*-\s*([a-z-]+):\s*(.*?)\s*(#.*)?$', line)
            if m:
                steps.append((m.group(1), m.group(2)))
            continue
        kept.append(line)
    return '\n'.join(kept), steps


def key_steps(steps):
    """The `- key:` values, and `printer-trace` steps as ('printer-trace', on|off), in order."""
    return [(v, s) for v, s in steps if v in ('key', 'printer-trace')]


class Run:
    """One `calcu --product c59 serve`."""

    def __init__(self, program=None):
        self.c = Calcu(program or calcu_path(), 'c59', json=False, cwd=ROOT)

    def close(self):
        self.c.close()

    def cmd(self, line, check=True):
        return self.c.send(line, check=check)['lines']

    def preset(self, path_or_text, name='t.ti59'):
        """A preset by its path, or (with a newline in it) by its text, through a file in tools/build/."""
        if '\n' in path_or_text:
            os.makedirs(BUILD, exist_ok=True)
            p = os.path.join(BUILD, 'calcu59-' + name)
            open(p, 'w', encoding='utf-8').write(path_or_text)
            path_or_text = p
        return self.cmd('preset ' + os.path.abspath(path_or_text))

    def step(self, verb, value):
        """A key step pressed (the reply when its last key is up), or printer-trace set."""
        if verb == 'printer-trace':
            self.cmd('printer-trace ' + value)
        else:
            self.cmd('key ' + value)

    def wait_idle(self):
        self.cmd('wait')

    def paper(self):
        r = self.c.send('paper text', check=False)
        return r['lines'] if r['done'] else []

    def program(self):
        """The keycodes of program memory up to the partition."""
        last = int(self.cmd('c59 partition')[0].split('.')[0])
        return [int(l.split()[1]) for l in self.cmd(f'c59 program 000 count={last + 1}')]


def play_until_last(path, steps=None, program=None):
    """Load the preset without its keys, then its key steps (or `steps`) but
    the last, each until idle. Returns (Run, the last step's value)."""
    text, all_steps = split_preset(path)
    ks = key_steps(steps if steps is not None else all_steps)
    run = Run(program)
    run.preset(text, os.path.basename(path))
    for verb, value in ks[:-1]:
        run.step(verb, value)
        if verb == 'key':
            run.wait_idle()
    return run, ks[-1][1]


def run_time(run, keys, limit=None):
    """`c59 run-time <keys>`: [(key, seconds, finished)], the total."""
    out = []
    lines = run.cmd('c59 run-time ' + keys)
    for l in lines[:-1]:
        w = l.split()
        out.append((w[0], float(w[1]), 'not' not in w[2:]))
    return out, float(lines[-1].split()[1])


def run_preset(path, steps=None, program=None):
    """Play the keys; time the last one (`c59 run-time`). Returns (seconds, printed lines, finished)."""
    run, last = play_until_last(path, steps, program)
    try:
        keys, total = run_time(run, last)
        return total, run.paper(), all(f for _, _, f in keys)
    finally:
        run.close()


def line_times(path, steps=None, limit=4000, program=None):
    """[(seconds after the last key, line)] of `c59 line-times`; stops 15 s
    after the last printed line once the machine is idle."""
    run, last = play_until_last(path, steps, program)
    try:
        run.step('key', last)
        out = []
        for l in run.cmd(f'c59 line-times limit={limit}s'):
            m = re.match(r'^(\d+\.\d+) (\d+\.\d+) \|(.*)\|$', l)
            if m:
                out.append((float(m.group(1)), m.group(3)))
        return out
    finally:
        run.close()


if __name__ == '__main__':
    args = sys.argv[1:]
    prog = calcu_path(args)
    t, lines, ok = run_preset(args[0], program=prog)
    print(f'{args[0]}: {t:7.2f} s  finished={ok}  lines={len(lines)} (non-blank {sum(1 for l in lines if l.strip())})')
    if '--lines' in args:
        print('\n'.join(lines))

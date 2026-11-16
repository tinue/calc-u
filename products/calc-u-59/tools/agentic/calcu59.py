"""Calc-U-59 through the harness `calcu` (LIB-11), for the tools in
tools/agentic/ (see README.md there). Replaces ti59file.py, which read the
2.3.0 state files itself (M-54, LIB-6 design §11.7): a preset is now loaded
by `calcu --product 59 serve` (`preset <file>`), in emulated time only.

The calcu program: --calcu PATH on the command line of a tool, else the
environment variable CALCU59_CALCU, else build/tests/library/harness/cli/calcu
of the repository (cmake --build build/tests --target calcu).

Timing (memory run_time_measurement, D-48): a key's run time is the time from
the key press until the display lights up again and no program runs (fB[0]
clear), as a stopwatch on a filmed calculator sees it; a Pause flash does not
end it, the blinking of an error display after the stop does not move it.
Sampled every 10 ms of emulated time (`wait 10ms`, `lcd`, `registers`); the
headless tool before M-54 sampled every 100 instructions, so a time moves by
at most one sample."""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))            # products/calc-u-59
REPO = os.path.dirname(os.path.dirname(ROOT))
sys.path.insert(0, os.path.join(REPO, 'library', 'harness', 'python'))
from calcu_serve import Calcu  # noqa: E402

SAMPLE_MS = 10
FLG_BUSY = 0x8000              # printer busy (the `flags` register)


def calcu_path(argv=None):
    """The calcu program: --calcu PATH in argv (removed from it), CALCU59_CALCU, or the build/tests tree."""
    if argv is not None and '--calcu' in argv:
        i = argv.index('--calcu')
        path = argv[i + 1]
        del argv[i:i + 2]
        return path
    return os.environ.get('CALCU59_CALCU', os.path.join(REPO, 'build', 'tests', 'library', 'harness', 'cli', 'calcu'))


def split_preset(path):
    """(the preset's text without its `keys:` sections, the steps of those
    sections in order as (verb, value)). The tools load the text and press
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
    """One `calcu --product 59 serve`."""

    def __init__(self, program=None):
        self.c = Calcu(program or calcu_path(), '59', json=False, cwd=ROOT)

    def close(self):
        self.c.close()

    def cmd(self, line, check=True):
        return self.c.send(line, check=check)['lines']

    def seconds(self):
        for l in self.cmd('ticks'):
            if l.startswith('seconds: '):
                return float(l.split()[1])
        return 0.0

    def preset(self, path_or_text, name='t.ti59'):
        """A preset by its path, or (with a newline in it) by its text, through a file in the scratch folder."""
        if '\n' in path_or_text:
            folder = os.path.join(ROOT, 'build')
            os.makedirs(folder, exist_ok=True)
            p = os.path.join(folder, 'agentic-' + name)
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

    def lit(self):
        rows = self.cmd('lcd')
        return bool(rows) and rows[0].strip() != '' and not rows[0].startswith('display off')

    def registers(self):
        out = {}
        for l in self.cmd('registers'):
            k, _, v = l.partition(' = ')
            out[k.strip()] = int(v, 16)
        return out

    def running(self):
        return bool(self.registers()['fB'] & 1)

    def scom0(self):
        """SCOM[0] as 16 nibbles, nibble 0 first."""
        v = int(self.cmd('peek TMC0501 scom 0 1')[0].split(': ')[1], 16)
        return [(v >> (4 * i)) & 0xF for i in range(16)]

    def paper(self):
        r = self.c.send('paper text', check=False)
        return r['lines'] if r['done'] else []

    def program(self):
        """The keycodes of program memory up to the partition."""
        last = int(self.cmd('ti59 partition')[0])
        return [int(l.split()[1]) for l in self.cmd(f'ti59 program 000 count={last + 1}')]


def dark_time(run, value, limit_s=4000):
    """Press the key step `value` and time it (module docstring). Returns
    (seconds, finished, ran): ran = the key ran a program (fB[0])."""
    t0 = run.seconds()
    run.step('key', value)
    on_at, went_dark, ran, stopped, quiet = t0, False, False, False, 0
    t = run.seconds()
    while t - t0 < limit_s:
        is_running = run.running()
        is_lit = run.lit()
        ran = ran or is_running
        stopped = stopped and not is_running
        if not is_lit and not stopped:
            went_dark, on_at, quiet = True, None, 0
        else:
            if on_at is None:
                on_at = t
            stopped = stopped or not is_running
            quiet = quiet + 1 if stopped else 0
            if quiet >= 25:                   # stopped for good, not a short gap (a quarter second)
                return ((on_at - t0) if went_dark else 0.0), True, ran
        run.cmd(f'wait {SAMPLE_MS}ms')
        t = run.seconds()
    return t - t0, False, ran


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


def run_preset(path, steps=None, program=None):
    """Play the keys; time the last one (dark_time). Returns (seconds, printed lines, finished)."""
    run, last = play_until_last(path, steps, program)
    try:
        t, ok, _ = dark_time(run, last)
        return t, run.paper(), ok
    finally:
        run.close()


if __name__ == '__main__':
    args = sys.argv[1:]
    prog = calcu_path(args)
    t, lines, ok = run_preset(args[0], program=prog)
    print(f'{args[0]}: {t:7.2f} s  finished={ok}  lines={len(lines)} (non-blank {sum(1 for l in lines if l.strip())})')
    if '--lines' in args:
        print('\n'.join(lines))

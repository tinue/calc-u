"""Time each printed line of a preset's run.

CLI: linetimes.py file.ti59 [--lines] [--limit S] [--calcu PATH]
  Loads the preset through calcu (tools/agentic/calcu59.py) and plays its
  keys; from the last key on, records the emulated time at which each
  printer line appears (`paper text` after each sample of 10 ms). Prints a
  summary by line kind (blank, calendar-style day row of 20 digits/spaces,
  other text) and the time of the last line. --lines lists every line with
  its time and its gap to the line before. --limit caps the run (default
  4000 emulated seconds)."""
import os
import re
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from calcu59 import SAMPLE_MS, calcu_path, play_until_last  # noqa: E402


def line_times(path, steps=None, limit=4000, program=None):
    """[(seconds after the last key, line)]. Stops 15 s after the last
    printed line once no program runs and the display is lit."""
    run, last = play_until_last(path, steps, program)
    try:
        t0 = run.seconds()
        seen = len(run.paper())
        run.step('key', last)
        out = []
        while True:
            now = run.seconds() - t0
            paper = run.paper()
            out += [(now, l) for l in paper[seen:]]
            seen = len(paper)
            if now >= limit or (out and now - out[-1][0] > 15 and not run.running() and run.lit()):
                break
            run.cmd(f'wait {SAMPLE_MS}ms')
        return out
    finally:
        run.close()


def kind(line):
    return 'blank' if not line.strip() else ('week' if re.fullmatch(r'[ \d]{20}', line) else 'text')


if __name__ == '__main__':
    args = sys.argv[1:]
    program = calcu_path(args)
    limit = float(args[args.index('--limit') + 1]) if '--limit' in args else 4000
    out = line_times(args[0], limit=limit, program=program)
    prev = 0; cat = {}
    for t, l in out:
        if '--lines' in args:
            print(f'{t:8.2f} {t - prev:5.2f} |{l}|')
        cat.setdefault(kind(l), []).append(t - prev); prev = t
    for k, v in cat.items():
        print(f'{k:6} n={len(v):3} avg {sum(v) / len(v):5.2f}s  total {sum(v):6.1f}s  min {min(v):.2f} max {max(v):.2f}')
    print('total', round(out[-1][0], 2) if out else None)

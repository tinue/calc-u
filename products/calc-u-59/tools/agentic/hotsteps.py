"""Where a running program spends its time: sampled program steps.

CLI: hotsteps.py file.ti59 [--until S] [--calcu PATH]
       Loads the preset through calcu (tools/agentic/calcu59.py) and samples
       the run (every millisecond of emulated time, from the last key to S
       emulated seconds, default 200). Prints the share of samples with the
       printer busy, the share spent in the ROM's print-wait spin loops, the
       Prg Src Flag values seen (non-zero 2/4/6 = fast mode) and the 25
       hottest program steps.
     hotsteps.py file.ti59 --window A B
       The sequence of program steps executed between A and B seconds, as
       step:samples (one sample per millisecond), e.g. to see what runs
       between two printed lines (take A and B from linetimes.py --lines).

The step is read from SCOM[0] (nibbles 5-7 register, nibble 4 byte), so a
multi-byte instruction shows up at the byte being fetched."""
import os
import sys
from collections import Counter
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from calcu59 import FLG_BUSY, calcu_path, play_until_last  # noqa: E402

PRINT_SPIN = (0x154C, 0x154D, 0x1425, 0x1426, 0x1427, 0x17C1, 0x17C2, 0x17C3, 0x15E4, 0x15E5, 0x1612, 0x1613)
SAMPLE = '1ms'   # one sample per millisecond of emulated time (about 97 instructions before M-54)


def step_of(scom0):
    return int('%d%d%d' % (scom0[7], scom0[6], scom0[5])) * 8 + scom0[4]


def start(path, program):
    run, last = play_until_last(path, program=program)
    t0 = run.seconds()
    run.step('key', last)
    return run, t0


def profile(path, until=200.0, program=None):
    run, t0 = start(path, program)
    n = busy = spin = 0; steps = Counter(); src = Counter()
    while run.seconds() - t0 <= until:
        run.cmd('wait ' + SAMPLE)
        r = run.registers(); sc = run.scom0(); n += 1
        busy += bool(r['flags'] & FLG_BUSY); spin += r['pc'] in PRINT_SPIN
        steps[step_of(sc)] += 1; src[sc[3]] += 1
    run.close()
    print(f'{path}: {until:.0f}s  printer busy {100 * busy / n:.0f}% of samples, '
          f'print-wait spin {100 * spin / n:.0f}%, Prg Src {dict(src)}')
    print('hottest program steps:', ', '.join(f'{k:03d}:{100 * v / n:.1f}%' for k, v in steps.most_common(25)))


def window(path, a, b, program=None):
    run, t0 = start(path, program)
    seq = []
    while True:
        run.cmd('wait ' + SAMPLE)
        t = run.seconds() - t0
        if t > b:
            break
        if t >= a:
            st = step_of(run.scom0())
            if seq and seq[-1][0] == st:
                seq[-1][1] += 1
            else:
                seq.append([st, 1])
    run.close()
    print(' '.join(f'{st:03d}:{k}' for st, k in seq))


if __name__ == '__main__':
    args = sys.argv[1:]
    program = calcu_path(args)
    if '--window' in args:
        i = args.index('--window')
        window(args[0], float(args[i + 1]), float(args[i + 2]), program)
    else:
        profile(args[0], float(args[args.index('--until') + 1]) if '--until' in args else 200.0, program)

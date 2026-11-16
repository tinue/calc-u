"""Program run time of every key step of a preset.

Times every key step that runs a program (fB[0] set) from the key press
until the display lights up again (calcu59.dark_time, through calcu), so
work that a label key such as A does before the timed start key is counted
too. Use this to compare programs that split their work differently between
preparation keys and the start key.

CLI: runtime.py file.ti59 [--keys "2 0 2 6 A ..."] [--calcu PATH]
  Loads the preset without its keys; --keys replaces the preset's `- key:`
  steps by these key IDs (docs/formats/preset.md §8.1, space separated, one
  step each; 2.3.0's matrix codes are gone). Prints step:seconds for each
  step that ran a program, and the total."""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from calcu59 import Run, calcu_path, dark_time, key_steps, split_preset  # noqa: E402


def key_runtimes(path, steps=None, program=None):
    """[(key step, seconds of program run time)]; 0 for a step that ran no program"""
    text, all_steps = split_preset(path)
    run = Run(program)
    try:
        run.preset(text, os.path.basename(path))
        res = []
        for verb, value in key_steps(steps if steps is not None else all_steps):
            if verb != 'key':
                run.step(verb, value)
                continue
            t, _, ran = dark_time(run, value)
            res.append((value, t if ran else 0.0))
        return res
    finally:
        run.close()


if __name__ == '__main__':
    args = sys.argv[1:]
    program = calcu_path(args)
    steps = [('key', k) for k in args[args.index('--keys') + 1].split()] if '--keys' in args else None
    r = key_runtimes(args[0], steps, program)
    print(' '.join(f'{k}:{t:.2f}' for k, t in r if t > 0), f' total {sum(t for _, t in r):.1f} s')

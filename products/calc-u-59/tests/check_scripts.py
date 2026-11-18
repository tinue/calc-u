"""C59-9 AC-13 (design §6, T11; CTest CalcU59Scripts, Longer): the scripts
that stay drive the harness and give what their documentation says.

- tools/coverage.py build boot digit5 add, then show: the ROM words of each
  scenario equal those 2.3.0's tool recorded at the base of M-59
  (tests/reference/scripts/coverage-base.json), with the key-detect range
  0627-0660 left out of both (the old tool left out an instruction of that
  range only with the idle flag set; the harness's `c59 coverage` leaves the
  range out, C59-9 D-7; the boot scenario is read with `all` and goes on
  into the idle loop).
- tools/fastmode_matrix.py label: for each `label` case the verdicts that
  2.3.0's tools/agentic/README.md documented (whether the run ended, the registers
  that differ) are those of 2.3.0's tool (tests/reference/scripts/
  fastmode-label-base.txt); the display samples and the Prg Src values seen
  are sampled at other times and are not compared.

CLI: check_scripts.py --calcu PATH   (run from products/calc-u-59)"""
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
args = sys.argv[1:]
calcu = args[args.index('--calcu') + 1]
env = dict(os.environ, CALCU=calcu)
failed = False

# coverage.py
subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'coverage.py'), 'build', 'boot', 'digit5', 'add'],
               check=True, env=env, stderr=subprocess.DEVNULL)
new = json.load(open(os.path.join(ROOT, 'tools', 'build', 'coverage.json')))
old = json.load(open(os.path.join(HERE, 'reference', 'scripts', 'coverage-base.json')))
key_detect = set(range(0x0627, 0x0661))
for name in ('boot', 'digit5', 'add'):
    a, b = set(old[name]) - key_detect, set(new.get(name, [])) - key_detect
    if a != b:
        failed = True
        print(f'coverage.py {name}: only before {sorted("%04X" % x for x in a - b)}, only now {sorted("%04X" % x for x in b - a)}')
    else:
        print(f'coverage.py {name}: {len(b)} words as before (without the key-detect range)')
show = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'coverage.py'), 'show', '0300', '0302'],
                      check=True, env=env, capture_output=True, text=True).stdout.split('\n')
want = [f'{a:04X} [{sum(a in set(v) for v in new.values()):3}]' for a in (0x300, 0x301, 0x302)]
if [l[:10] for l in show[:3]] != want:
    failed = True
    print('coverage.py show:', show[:3], 'wanted', want)


# fastmode_matrix.py
def verdicts(text):
    out, case = {}, None
    for line in text.split('\n'):
        m = re.match(r'^(SAME|DIFF) (.{32}) ', line)
        if m:
            case = m.group(2).strip()
            out[case] = []
        elif case and re.match(r'^\s+(ok|R\d\d):', line):
            out[case].append(line.strip())
    return out


got = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'fastmode_matrix.py'), 'label'],
                     check=True, env=env, capture_output=True, text=True).stdout
was = open(os.path.join(HERE, 'reference', 'scripts', 'fastmode-label-base.txt'), encoding='utf-8').read()
if verdicts(got) != verdicts(was):
    failed = True
    print('fastmode_matrix.py label differs:\n' + got)
else:
    print(f'fastmode_matrix.py label: {len(verdicts(got))} cases as before')
sys.exit(1 if failed else 0)

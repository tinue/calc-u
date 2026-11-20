"""CTest CalcU59Scripts (Longer): the scripts that stay drive the harness and
give what their references hold (tests/references/scripts/).

- tools/coverage.py build boot digit5 add: the ROM words of each scenario
  equal those of coverage.json; `show 0300 0302` gives each word's count of
  scenarios.
- tools/fastmode_matrix.py label: for each `label` case the verdicts
  (whether the run ended, the registers that differ) equal those of
  fastmode-label.txt; the display samples and the Prg Src values seen are not
  compared.

CLI: check_scripts.py --calcu PATH [--record FOLDER]   (run from products/calc-u-59)
--record writes the references into FOLDER (a draft) instead of comparing.
Without the references it exits with 77 (CTest: skipped)."""
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
REFS = os.path.join(HERE, 'references', 'scripts')
args = sys.argv[1:]
calcu = args[args.index('--calcu') + 1]
record = args[args.index('--record') + 1] if '--record' in args else None
env = dict(os.environ, CALCU=calcu)
if not record and not os.path.isdir(REFS):
    print('SKIP: references re-recorded, waiting for agreement')
    sys.exit(77)
failed = False

# coverage.py
subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'coverage.py'), 'build', 'boot', 'digit5', 'add'],
               check=True, env=env, stderr=subprocess.DEVNULL)
built = os.path.join(ROOT, 'tools', 'build', 'coverage.json')
new = json.load(open(built))
if record:
    os.makedirs(record, exist_ok=True)
    shutil.copyfile(built, os.path.join(record, 'coverage.json'))
else:
    old = json.load(open(os.path.join(REFS, 'coverage.json')))
    for name in ('boot', 'digit5', 'add'):
        a, b = set(old[name]), set(new.get(name, []))
        if a != b:
            failed = True
            print(f'coverage.py {name}: only before {sorted("%04X" % x for x in a - b)}, only now {sorted("%04X" % x for x in b - a)}')
        else:
            print(f'coverage.py {name}: {len(b)} words as before')
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
if record:
    open(os.path.join(record, 'fastmode-label.txt'), 'w', encoding='utf-8').write(got)
    print(f'recorded into {record}')
    sys.exit(1 if failed else 0)
was = open(os.path.join(REFS, 'fastmode-label.txt'), encoding='utf-8').read()
if verdicts(got) != verdicts(was):
    failed = True
    print('fastmode_matrix.py label differs:\n' + got)
else:
    print(f'fastmode_matrix.py label: {len(verdicts(got))} cases as before')
sys.exit(1 if failed else 0)

# Automated tests with Calc-U-1600

Write pytest tests that run on an emulated PC-1500, PC-1500A or PC-1600: press keys, type lines,
plug modules and media, then check the display, memory, cards or the BASIC program. Each test gets
a fresh machine; many run in parallel.

The test driver began as the one "spellbound" wrote for the tests of the superRAM module.
It is developed further together with spellbound, in this repository.

Contents: [1 Setup](#1-setup) · [2 A test project](#2-a-test-project) ·
[3 Example: MODE 1 and memory modules](#3-example-mode-1-and-memory-modules) ·
[4 Example: RENUM on the PC-1500](#4-example-renum-on-the-pc-1500) ·
[5 Example: the serial ports](#5-example-the-serial-ports) · [6 Paths](#6-paths) ·
[7 Reference](#7-reference) · [Appendix: how it works](#appendix-how-it-works)

## 1 Setup

You need the repository checked out and its harness program `calcu` built. The ROM images are not
in the repository; a script fetches them.

| | macOS | Linux, WSL |
|---|---|---|
| Compiler, build tools | Xcode command-line tools, CMake 3.21+, Ninja | GCC 11+, CMake 3.21+, Ninja |
| Python, pytest | [uv](https://docs.astral.sh/uv/) (brings Python and pytest) | uv |
| `libsharpdx` (BASIC files) | in the repository | fetched by a script |

**1. Tools.**

```sh
# macOS (with Homebrew)
xcode-select --install
brew install cmake ninja uv

# Linux, WSL (Ubuntu 24.04)
sudo apt install build-essential cmake ninja-build zlib1g-dev git curl
curl -LsSf https://astral.sh/uv/install.sh | sh
```

On WSL, check out into the Linux file system (`~/…`), not into `/mnt/c/…`: builds and tests there
are many times slower.

**2. The repository and the ROMs.**

```sh
git clone https://github.com/tinue/calc-u.git ~/calc-u
cd ~/calc-u
products/calc-u-1600/roms/fetch_roms.sh
(cd products/calc-u-1600 && tools/fetch_sharpdx.sh)     # Linux and WSL only
```

**3. Build `calcu`.** Only the harness, no apps and no tests:

```sh
cmake -S . -B build/tools -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCALCU_FRONTEND_QT=OFF -DCALCU_PRODUCT_59=OFF -DCALCU_TESTS=OFF
cmake --build build/tools --target calcu
```

The driver finds `build/tools/library/harness/cli/calcu` by itself. A `calcu` elsewhere: set
`CALCU` to its path.

**4. Check.** The driver's own tests:

```sh
uv run --no-project --with pytest --with pytest-xdist pytest -n auto products/calc-u-1600/tools/machine
```

**Later updates:** `git pull`, `fetch_roms.sh` again (it skips what it has), then step 3's
`cmake --build` line.

## 2 A test project

A test project is a folder of its own, outside the checkout:

```
~/pc-tests/
├── pytest.ini          where the driver is; names for your folders
├── conftest.py         one line: loads the driver
├── pyproject.toml      made by uv: pytest and pytest-xdist
├── programs/           your files, wherever you like in the project
│   └── jumps.bas
├── test_mode1.py
└── test_renum.py
```

**1. Make it.**

```sh
mkdir ~/pc-tests && cd ~/pc-tests
uv init --bare
uv add --dev pytest pytest-xdist
mkdir programs
```

**2. `pytest.ini` and `conftest.py`.** `pytest.ini` names the driver's two folders in your
checkout and, if you like, names for your own folders:

```ini
[pytest]
pythonpath = /home/me/calc-u/products/calc-u-1600/tools/machine
             /home/me/calc-u/library/harness/python
calcu_paths =
    programs = programs
```

`conftest.py` loads the driver's pytest plugin, `calcu_pytest`: the fixture `make_machine` and the
path names of [6 Paths](#6-paths). It has to be this file, not `-p calcu_pytest` in `addopts`:
pytest loads `-p` plugins before it reads `pythonpath`.

```python
pytest_plugins = ["calcu_pytest"]
```

**3. A first test,** `test_hello.py`:

```python
def test_hello(make_machine):
    machine = make_machine().type('PRINT "=";6*7')
    assert "= 42" in machine.lcd
```

**4. Run.**

```sh
uv run pytest -n auto          # all tests, in parallel
uv run pytest -k hello         # some of them
```

How a test runs:

- `make_machine(...)` makes a machine in a work folder of its own (under pytest's temporary
  folders). Nothing runs yet.
- Steps (`key`, `type`, `wait`, `expect`, …) are collected. Each `key` and `type` waits until the
  machine is idle again.
- A read (`lcd`, `listing()`, `card()`, `run()`, …) starts `calcu` the first time, then runs the
  collected steps and returns the result. Steps after a read continue from there.
- A failed `expect` or a refused step raises `MachineError` with the reason: the test fails with
  the screen in the message.

A fresh machine always starts the same way: a PC-1600 in RUN mode, a PC-1500 or PC-1500A in PRO
mode, both with `>` on the screen. So a test knows the mode without looking.

## 3 Example: MODE 1 and memory modules

The examples are a test project in the checkout, [`docs/testing/example/`](example/), that runs
as it is: `cd products/calc-u-1600/docs/testing/example`, then
`uv run --no-project --with pytest --with pytest-xdist pytest -n auto`.

On the PC-1600, `MODE 1` switches to PC-1500 mode. It needs the RAM the PC-1500 code sees to be one
flat block of at most 16 KB. A banked PC-1600 module (the bank number on the data bus, Port 28H)
refuses it with `ERROR 110`, and so does more than one module's RAM.

| Slot 1 | Slot 2 | `MODE1` |
|---|---|---|
| — | — | accepted |
| CE-151, CE-155 or CE-161 | — | accepted |
| CE-163F or CE-1638 | — | accepted: the ROM can't detect their RAM and ignores the card |
| — | CE-161, CE-163F or CE-1638 | accepted (their banks are ignored) |
| CE-1600M (32 KB) | — | ERROR 110 |
| — | CE-1600M, CE-1601M or superRAM | ERROR 110 |
| CE-155 | CE-163F or CE-1638 | ERROR 110 |

The CE-163F and CE-1638 are PC-1500 modules: they take their bank from the address bus, so the
PC-1600 doesn't treat them as banked. In slot 1 their bank changes all the time, the ROM finds no
RAM, and `MEM` stays as it is without a module.

`test_mode1.py`:

```python
import pytest

REFUSED, ACCEPTED = True, False
CASES = {
    "none":               ({}, ACCEPTED),
    "CE-161 in 1":        ({"slot_1": "CE-161"}, ACCEPTED),
    "CE-163F in 1":       ({"slot_1": "CE-163F"}, ACCEPTED),
    "CE-163F in 2":       ({"slot_2": "CE-163F"}, ACCEPTED),
    "CE-1600M in 1":      ({"slot_1": "CE-1600M"}, REFUSED),
    "CE-1600M in 2":      ({"slot_2": "CE-1600M"}, REFUSED),
    "superRAM in 2":      ({"slot_2": "superRAM"}, REFUSED),
    "CE-155 and CE-1638": ({"slot_1": "CE-155", "slot_2": "CE-1638"}, REFUSED),
    "a card file in 2":   ({"slot_2": "$c1600/examples/dwx/CE-1601M - Progs.module.yaml"}, REFUSED),
}


@pytest.mark.parametrize("slots, refused", CASES.values(), ids=CASES.keys())
def test_mode1(make_machine, slots, refused):
    machine = make_machine(model="PC-1600", **slots)
    machine.key("mode").type("NEW0").key("mode")     # NEW0 needs PRO mode; back to RUN
    machine.type("MODE1")
    if refused:
        machine.expect("ERROR 110")
    else:
        machine.not_expect("ERROR")
    machine.run()
```

- A slot takes a bundled module by its name (`CE-1600M`, …) or a module file. The last case
  uses a card with content from the checkout's examples, by the path name `$c1600`
  ([6 Paths](#6-paths)).
- `expect` and `not_expect` take effect at the next read; `run()` is a read without a result.
- One test function, nine machines: pytest shows each case by its id, and `-n auto` runs them at
  the same time.

## 4 Example: RENUM on the PC-1500

The repository has a ROM extension that adds `RENUM [new][,[old][,step]]` to PC-1500 BASIC, as a
module on the 60-pin bus at &8800. It is the debugger's *PC-1500 ROM extension* example; the test
uses its built ROM where it is, `$c1600/tests/fixtures/dap/pc1500-bus-rom/rom.bin`
([6 Paths](#6-paths)).

RENUM gives the lines from `old` on (default: the first) the numbers `new`, `new+step`, …
(defaults 10 and 10) and changes every `GOTO`, `GOSUB`, `THEN`, `ON…GOTO` and `RESTORE` to match. On
an error the program stays as it was: `ERROR 19` (a number 0 or above 65279), `ERROR 11` (no line
`old`), `ERROR 30` (the last new number above 65279).

`programs/jumps.bas`, a program with every kind of jump (`$programs/jumps.bas` in the test, by the
name from `pytest.ini`):

```basic
10 REM RENUM DEMO
20 RESTORE 80
30 READ N
40 ON N+1 GOTO 70,50,50,50
50 GOSUB 90
60 IF N THEN 30
70 PRINT "DONE":END
80 DATA 3,2,1,0
90 PRINT N:RETURN
```

The tests try every combination of the model, how the program gets in, and the RENUM command:

| | Choices |
|---|---|
| model | PC-1500, PC-1500A |
| program entry | typed line by line, `load_basic` (straight into memory), `type_basic` (typed from the file) |
| command | three that renumber, three that are refused |

`test_renum.py`:

```python
from pathlib import Path

import pytest

# listing() gives the lines as the detokenizer writes them: no blank between a
# variable or number and the keyword after it (ON N+1GOTO, IF NTHEN).
LISTED = [
    "10 REM RENUM DEMO", "20 RESTORE 80", "30 READ N",
    "40 ON N+1GOTO 70,50,50,50", "50 GOSUB 90", "60 IF NTHEN 30",
    '70 PRINT "DONE":END', "80 DATA 3,2,1,0", "90 PRINT N:RETURN",
]
PROGRAM = Path(__file__).parent / "programs"
RENUM_ROM = "$c1600/tests/fixtures/dap/pc1500-bus-rom/rom.bin"
RENUMBERED = {
    "RENUM": LISTED,                          # 10, 20, ... again
    "RENUM 100,,10": [
        "100 REM RENUM DEMO", "110 RESTORE 170", "120 READ N",
        "130 ON N+1GOTO 160,140,140,140", "140 GOSUB 180", "150 IF NTHEN 120",
        '160 PRINT "DONE":END', "170 DATA 3,2,1,0", "180 PRINT N:RETURN",
    ],
    "RENUM 1000,50,100": [                    # from line 50 on only
        "10 REM RENUM DEMO", "20 RESTORE 1300", "30 READ N",
        "40 ON N+1GOTO 1200,1000,1000,1000", "1000 GOSUB 1400", "1100 IF NTHEN 30",
        '1200 PRINT "DONE":END', "1300 DATA 3,2,1,0", "1400 PRINT N:RETURN",
    ],
}
REFUSED = {"RENUM 100,,0": "ERROR 19", "RENUM 60000,,1000": "ERROR 30", "RENUM 100,55": "ERROR 11"}


@pytest.fixture(params=["PC-1500", "PC-1500A"])
def machine(request, make_machine):
    renum = {"file": RENUM_ROM, "address": 0x8800, "pv": 0}
    return make_machine(model=request.param, bus_rom=[renum])


@pytest.fixture(params=["typed", "load_basic", "type_basic"])
def with_program(request, machine):
    machine.key("cl").type("NEW0")             # a fresh PC-1500 is in PRO mode
    if request.param == "typed":
        for line in (PROGRAM / "jumps.bas").read_text().splitlines():
            machine.type(line)
    else:
        getattr(machine, request.param)("$programs/jumps.bas")
    return machine.key("mode")                 # to RUN mode


@pytest.mark.parametrize("command", RENUMBERED)
def test_renum(with_program, command):
    assert with_program.type(command).listing() == RENUMBERED[command]


@pytest.mark.parametrize("command", REFUSED)
def test_renum_refused(with_program, command):
    machine = with_program.type(command).expect(REFUSED[command])
    assert machine.listing() == LISTED
```

Two fixtures with two and three choices, two tests with three commands each: 36 runs. A fixture
with `params` multiplies every test that uses it.

`listing()` gives the lines as the program's detokenizer writes them, which isn't always as they
were typed: no blank between a variable or number and the keyword after it (`ON N+1GOTO`,
`IF NTHEN`). Write the expected lines in that form.

## 5 Example: the serial ports

The PC-1600 has its own RS-232C port, `COM1:`. The PC-1500 and PC-1500A get one with the CE-158
interface (the PC-1600 too, in MODE 1, without a CE-1600P). A test plays the other side of the
cable with harness commands: `rs232c` is the PC-1600's port, `ce158` the CE-158's.

| Command, from `command()` | Does |
|---|---|
| `c1600 serial rs232c\|ce158 feed <file> [hold=<n>]` | sends the file's bytes to the machine, one per character time at the speed the port is set to, after `n` character times of nothing |
| `c1600 serial rs232c\|ce158 capture <file>` | writes what the machine sends into the file; the file is complete when `calcu` ends |
| `c1600 serial rs232c\|ce158 report` | with `send()`: `baud` (0 when not set), `used`, `fed` and `taken` (bytes fed, bytes the machine took), `sent` (everything the port sent, as text: printable ASCII and LF as they are, CR left out, any other byte `<XX>`); for the CE-158 also `printer`, its parallel port |

The other side holds CTS, DSR and CD on: the PC-1600 sends without a `SNDSTAT` of its own. The
BASIC side, as the examples set it up:

| | PC-1600, `COM1:` | CE-158 |
|---|---|---|
| speed, bits, parity, stop bits | `SETCOM "COM1:",9600,8,N,1,N,N` (then XON/XOFF and shift in/out, both off) | `SETCOM 1200,8,N,1` (50 to 2400 baud) |
| handshake lines | automatic | `OUTSTAT 0`: RTS and DTR on (off at power-on) |
| `INPUT` from the port, `LPRINT` to it | `SETDEV "COM1:",KI,PO`; line ends with `PCONSOLE "COM1:",0,2` (CR LF) | `SETDEV KI,PO` (`DO` for `PRINT`) |
| a program out, a program in | `SAVE"COM1:",A` (ASCII: CR LF after each line, 1AH at the end), `LOAD"COM1:"` | `SETDEV CO`, then `CSAVE "name"`; `SETDEV CI`, then `CLOAD "name"` |

`test_serial.py`, with `programs/greet.bas`, a console program:

```basic
10 SETCOM "COM1:",9600,8,N,1,N,N
20 PCONSOLE "COM1:",0,2
30 SETDEV "COM1:",KI,PO
40 LPRINT "NAME?"
50 INPUT N$
60 LPRINT "HELLO ";N$
70 END
```

```python
from pathlib import Path

PROGRAMS = Path(__file__).parent / "programs"
COM1 = 'SETCOM "COM1:",9600,8,N,1,N,N'
LISTED = (PROGRAMS / "greet.bas").read_text().splitlines()   # listing() gives these lines as they are


def test_console(make_machine, tmp_path):
    name = tmp_path / "name.txt"
    name.write_bytes(b"WORLD\r")                 # INPUT takes the line up to CR
    machine = make_machine(model="PC-1600")
    machine.key("mode").type("NEW0").load_basic("$programs/greet.bas").key("mode")
    machine.type("RUN").wait(1)                  # NAME? is out, INPUT waits
    machine.command(f"c1600 serial rs232c feed {name}").wait(1)
    report = machine.send("c1600 serial rs232c report")
    assert report["sent"].splitlines() == ["NAME?", "HELLO WORLD"]
    assert report["taken"] == report["fed"] == 6


def test_save(make_machine, tmp_path):
    sent = tmp_path / "sent.bas"
    machine = make_machine(model="PC-1600")
    machine.key("mode").type("NEW0").load_basic("$programs/greet.bas").key("mode")
    machine.command(f"c1600 serial rs232c capture {sent}")
    machine.type(COM1).type('SAVE"COM1:",A').run()   # type waits until the SAVE has ended
    machine.close()                              # the capture file is complete when calcu ends
    data = sent.read_bytes()
    assert data.endswith(b"\r\n\x1a")            # CR LF after each line, 1AH at the end
    assert [line.rstrip() for line in data[:-3].decode("ascii").split("\r\n")] == LISTED


def test_load(make_machine, tmp_path):
    file = tmp_path / "greet.txt"
    file.write_bytes(b"".join(line.encode("ascii") + b"\r\n" for line in LISTED) + b"\x1a")
    machine = make_machine(model="PC-1600")
    machine.key("mode").type("NEW0").key("mode").type(COM1)
    machine.command('type LOAD"COM1:"')          # the harness's type: no wait
    machine.command(f"c1600 serial rs232c feed {file} hold=100").wait()
    assert machine.listing() == LISTED


def test_ce158_csave_cload(make_machine, tmp_path):
    sent = tmp_path / "sent.bin"
    one = make_machine(model="PC-1500A").command("attach interface CE-158")
    one.key("cl").type("NEW0").type('10 PRINT "HELLO"').type("20 GOTO 10").key("mode")
    one.command(f"c1600 serial ce158 capture {sent}")
    one.type("SETCOM 1200,8,N,1").type("OUTSTAT 0").type("SETDEV CO").type('CSAVE "P"')
    listing = one.listing()
    one.close()
    two = make_machine(model="PC-1500A").command("attach interface CE-158")
    two.key("cl").type("NEW0").key("mode")
    two.type("SETCOM 1200,8,N,1").type("OUTSTAT 0").type("SETDEV CI")
    two.command('type CLOAD "P"')
    two.command(f"c1600 serial ce158 feed {sent} hold=10").wait()
    assert two.listing() == listing == ['10 PRINT "HELLO"', "20 GOTO 10"]
```

- **A console:** the program runs, so `wait()` without seconds wouldn't wait for it: a running
  BASIC program reads the keyboard for BREAK and counts as idle. `wait(1)` gives it the time to
  write `NAME?` and to take the line. `report` shows what it sent.
- **A program out:** `type` waits until the `SAVE` has ended. The capture file is complete when
  `calcu` ends: `run()` sends the collected steps, then `close()` ends it.
- **A program in:** the file arrives after the `LOAD` has started (`hold=`): bytes that come
  before are not loaded. The harness's own `type` doesn't wait, so the feed follows at once; then
  `wait()` lasts until the `LOAD` has ended. A `LOAD` that gets nothing waits up to an hour of
  machine time, about a minute.
- **The CE-158:** the driver has no option for it; `command("attach interface CE-158")` plugs it
  in. `CLOADa` needs two seconds of silence between the lines, which `feed` doesn't give: the
  test sends the program the binary way, from one machine to the next.

## 6 Paths

The driver brings no modules, media or programs of its own. A test runs on what Calc-U-1600
ships, and on what the test project brings:

- **Shipped:** the bundled modules by name (`CE-1600M`, `superRAM`); the bundled media and the
  examples as files of the checkout, by `$c1600`: the module definitions, `blank.floppy.yaml`,
  `formatted.floppy.yaml` and `empty.cassette.yaml` in `$c1600/frontends/qt6/resources/cards/`,
  the examples in `$c1600/examples/`.
- **Brought:** anything else is a file of the test project, whatever the medium: a module still
  being built, a floppy disk or a cassette with your programs, a ROM image, a program. Copy the
  nearest shipped file and change it, or save one in the app (Name & Save, then `$saved`) and
  copy it into the project.

A test names these files and folders by path. A path is one of three:

| Path | Found in | Example |
|---|---|---|
| absolute | as it is | `/Volumes/STICK/cards/test.module.yaml` |
| `$name/…` | the folder the name stands for | `$c1600/examples/dwx/Progs.floppy.yaml` |
| anything else | the test project root | `programs/jumps.bas`, `MyFloppy.floppy.yaml` |

The **test project root** is the folder of `pytest.ini`: the same for every test file, whatever
folder pytest is started from.

The names:

| Name | Stands for | Known from |
|---|---|---|
| `$project` | the test project root | `pytest.ini` |
| `$calcu` | the checkout's root, `~/calc-u` | where the driver is (`pythonpath`) |
| `$c1600` | the product in the checkout, `$calcu/products/calc-u-1600` | the same |
| `$saved` | the save folder of the Calc-U-1600 app, by default `~/Calc-U-1600`: the media you saved there with Name & Save | the app's settings file, `[storage] save-folder`; an error if the app was never started |
| `$work` | this machine's work folder (also `machine.workdir`) | the driver |
| your own | a folder you name in `pytest.ini` | `calcu_paths` |

Names of your own, one per line, relative to the root or absolute:

```ini
calcu_paths =
    cards = test-cards
    stick = /Volumes/STICK
```

An environment variable `CALCU_PATH_<NAME>` wins over a name, for example on another computer:
`CALCU_PATH_STICK=/media/me/STICK uv run pytest`. An unknown name raises `MachineError`.

- A **slot** value without `/` and without `.module.yaml` is a bundled module's name (`CE-1600M`,
  `superRAM`). Anything else is a path: `cards/my-ram.module.yaml`, `$cards/superram-full.module.yaml`.
  The other media are always paths: `floppy("$c1600/frontends/qt6/resources/cards/formatted.floppy.yaml")`,
  `floppy("disks/progs.floppy.yaml")`.
- `host_drive` is a folder on your computer, found like any path; usually an absolute one or a
  per-test folder: `host_drive=tmp_path / "drive"`.
- **Files a test makes** belong in pytest's per-test folder `tmp_path` or in `$work`. A card saved
  with `card(2)` is `$work/slot-2.module.yaml`, to put into the next machine; its bytes alone, a
  raw image, are `card(2)` written to a file ([7 Reference](#7-reference)).

## 7 Reference

`make_machine(**options)` → a `Machine` in a work folder of its own. A suite with steps of its own
overrides the fixture `machine_class` in its `conftest.py`:

```python
from machine import Machine

class MyMachine(Machine):
    def new0(self):
        return self.key("mode").type("NEW0").key("mode")

@pytest.fixture
def machine_class():
    return MyMachine
```

A machine made outside `make_machine`, for example in a session-scoped fixture, takes the project
root and the names itself: `MyMachine(workdir, project=config.rootpath, paths=path_names(config))`
(`from calcu_pytest import path_names`).

Options:

| Option | Values | Default |
|---|---|---|
| `model` | `PC-1500`, `PC-1500A`, `PC-1600` | `PC-1600` |
| `slot_1`, `slot_2` | a bundled module's name or a path; `slot_2` on the PC-1600 only | empty |
| `printer` | `CE-150`, `CE-1600P`, `CE-1600P:old` | none |
| `host_drive` | a folder, the PC-1600's drive `S3:` | none |
| `bus_rom` | a list of `{"file", "address", "pv"}`: ROMs on the 60-pin bus, in plug order | none |

Steps, collected until the next read:

| Step | Does |
|---|---|
| `key(names)` | presses keys by name, several separated by blanks: `key("shift mode")`. Each key is held until the machine has taken it, then the machine runs until idle. A finger's press that the program may miss: `command("key-down y").wait(0.1).command("key-up y")` (key IDs; ON/BREAK is `on` or `break`) |
| `type(text)` | types one line and ENTER |
| `wait(seconds=None)` | waits in machine time; without seconds until idle |
| `expect(text)`, `not_expect(text)` | the next read fails unless / if a screen line holds text (`/regex/` too) |
| `load(path)` | machine code straight into memory (its header gives the address) |
| `load_basic(path)`, `type_basic(path)` | a BASIC listing: straight into memory, or typed line by line |
| `floppy(path, side=0)`, `cassette(path)`, `eject(place)` | inserts or ejects (`floppy`, `cassette`) a medium |
| `command(line)` | any harness command |

Reads:

| Read | Returns |
|---|---|
| `run()` | runs the steps; the replies |
| `send(line)` | runs the steps, then any harness command; its result (the command's `--json` value), e.g. `send("c1600 serial rs232c report")["sent"]` |
| `wait_for(text, timeout=60)` | runs the steps, then waits in machine time until a screen line holds text (`/regex/` too); for work the idle wait can't see the end of: a running BASIC program or machine code started by `CALL` counts as idle, since it reads the keyboard (a direct `LOAD`, `SAVE` or `COPY` does not: `wait()` lasts until it has ended) |
| `lcd` | the screen's text lines |
| `display()` | the screen's dots (PC-1600: 156 × 32) |
| `listing()` | the BASIC program as text lines, the detokenizer's (MODE 0 on the PC-1600) |
| `card(slot)` | a module's bytes, in bank order; also saved as `$work/slot-<n>.module.yaml`. As a raw image, the form a preset's `slot-N-rom:` loads into a ROM module: `(tmp_path / "slot-2.bin").write_bytes(machine.card(2))` |
| `peek(target, space, address, count, bank=, vbank=)` | memory bytes, without side effects. On the PC-1600, `bank=3, vbank=1` reads the hidden ROM bank 3b and `vbank=0` bank 3, whatever port 3DH selects now; the banks are in [the harness's PC-1500/1600 commands](../developer/Harness.md) |
| `expect_memory(target, space, address, values, …)` | raises unless the bytes match |
| `save_state(path)`, `restore_state(path)` | the whole machine |
| `run_to(target, address, timeout)` | runs until the CPU reaches the address; seconds taken |

Key names: a letter, digit or `+ - * / = ( ) .`; `enter`, `space`, `cl`, `mode`, `shift`, `def`,
`sml`, `rcl`, `rsv`, `off`, `up`, `down`, `left`, `right`, `f1`–`f6`, `on` (or `break`); on the
PC-1600 also `ctrl`, `kbii`, `bs`.

## Appendix: how it works

```mermaid
flowchart LR
    T[your tests] --> F["make_machine<br/>(calcu_pytest)"] --> M["Machine<br/>(machine.py)"]
    M --> S["Session<br/>(calcu_machine.py)"] -- "one command per line" --> C["calcu serve<br/>(c1600)"]
    C --> E[emulated machine<br/>ROMs, modules, media]
```

| Part | Where | Does |
|---|---|---|
| `calcu_pytest` | `products/calc-u-1600/tools/machine/` | the pytest plugin: the `make_machine` fixture, the path names |
| `Machine` | same folder, `machine.py` | the PC-1500/1600 part: models, slots, media, bus ROMs, `card()`, `listing()` |
| `cardimage.py` | same folder | reads saved module files into bytes, and the RAM disk on them |
| `Session` | `library/harness/python/calcu_machine.py` | the part common to every Calc-U product: steps, reads, path names, starting and ending `calcu` |
| `calcu` | `library/harness/cli/`, built into `build/tools/` | the headless runner: the emulator without a window |

- `conftest.py` loads `calcu_pytest`; `make_machine` makes the work folder and a `Machine` with the
  project root and the path names. The first read starts `calcu --product c1600 serve` with the
  work folder's `save/` and `out/` as its save and output folders.
- The `Machine` writes a short preset, `setup.pc1600`, with the model, slots, printer, host drive
  and bus ROMs, and loads it; the machine starts cold.
- Each step and read is one harness command line: `key mode`, `type NEW0`, `wait`,
  `expect ERROR 110`, `floppy { file: …, side: 0 }`, `c1600 list-basic`. `calcu` answers each with
  one reply line; a refusal becomes `MachineError`.
- `calcu` runs the emulation as fast as it can, in machine time: `wait 2s` is two seconds for the
  machine, much less for you.
- After a failure, the work folder holds the preset and what `calcu` wrote: pytest keeps the
  folders of its last three runs (`pytest --basetemp=…` puts them somewhere fixed).

The commands, presets and files behind it are specified in
[the harness](../../../../docs/formats/harness.md) and
[its PC-1500/1600 commands](../developer/Harness.md), [presets](../../../../docs/formats/preset.md),
[modules](../../../../docs/formats/modules.md) and [media](../../../../docs/formats/media.md).

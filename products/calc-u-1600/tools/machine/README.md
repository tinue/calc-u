# pytest driver for the PC-1600

A test machine for pytest suites, on `calcu serve`. It took over the driver of an external
test package written for Calc-U-1600 0.7.0 (`pc1600_cli` and generated presets); the test cases'
API stays.

| File | Content |
|------|---------|
| `machine.py` | Class `Machine`: the PC-1600 with its slots, `load()`, `card()` |
| `cardimage.py` | Parser for saved cards and the RAM disk on them (no machine needed) |
| `cards/` | `ram-256k`, `ram-512k`, `ram-1m`: superRAM with 8, 16, 32 vertical banks |
| `test_driver.py` | Tests of the driver and the parser |
| `../../../../library/harness/python/calcu_machine.py` | Class `Session`, the product-neutral part |

## Run

Needs Python 3, pytest and a built `calcu` (`cmake --build build/all --target calcu`).

```sh
export CALCU=/path/to/build/all/library/harness/cli/calcu   # default: build/tests, build/all or build/tools of this checkout (calcu_serve.find_calcu)
uv run --no-project --with pytest==8.3.5 --with pytest-xdist==3.6.1 pytest -n auto products/calc-u-1600/tools/machine
```

A suite elsewhere puts this folder and `library/harness/python` on its `pythonpath`. The ctest entry
is `CalcU1600MachineDriver` (Longer).

## API

```python
machine = Machine(tmp_path, slot_2="ram-1m")      # slot_1, slot_2, host_drive, model, printer
machine.type('PRINT "=";6*7').wait(1)
assert "= 42" in machine.lcd
image = machine.card(2)
```

- Steps: `key(names)`, `type(text)` (the line and ENTER), `wait(seconds=None)` (without seconds
  until idle), `expect(text)`, `not_expect(text)` (text or `/regex/`), `load(path)`
  (`c1600 load-code`), `floppy(path, side=0)`, `cassette(path)`, `eject(place)` (`floppy` or
  `cassette`), `command(line)` (any harness command), `resume()`.
- Reads: `run()`, `lcd` (text rows), `display()` (`rows` of 0/1, 156 × 32), `card(slot)` (bytes in
  bank order), `peek(target, space, address, count, bank=, vbank=)` → bytes,
  `expect_memory(...)`, `save_state(path)`, `restore_state(path)`,
  `run_to(target, address, timeout)` (stops there paused, or raises; seconds of machine time).
  Targets and spaces: `SC7852` `memory`/`io`/`io-written`, `LH5803` `me0`/`me1`/`me1-written`.
- A slot is the name of a file in `cards/` or of a bundled module (`CE-1600M`, …), or a module file of
  your own: a `Path`, or a string that ends in `.module.yaml` or holds a `/`. A missing file
  raises `MachineError`.
- A relative path (a slot file, `host_drive`, `load`, `floppy`, `cassette`) is relative to the
  Machine's directory, as in the 0.7.0 driver, whose preset lay there; not to the current directory.
- `printer="CE-1600P"` (or `CE-150`, `CE-1600P:old`) plugs a printer; a floppy needs the CE-1600P,
  a cassette either printer.
- A refused command or a failed check raises `MachineError` with the harness's reason.
- Each Machine needs its own directory; it holds the run's save and output folders.
  `close()` ends the `calcu` process (a finalizer does it otherwise).

### Your own cards and media

```python
card = MY_CARDS / "work.module.yaml"             # a definition of your own, or a card saved before
machine = Machine(tmp_path, slot_2=card, printer="CE-1600P").floppy("disks/progs.floppy.yaml")
image = machine.card(2)                          # also saved as tmp_path/slot-2.module.yaml
again = Machine(tmp_path / "next", slot_2=tmp_path / "slot-2.module.yaml")   # carries on from there
```

Bank 0 of a RAM card is the PC-1600's expansion memory (module header, BASIC area), so its head
changes at power on; the RAM disk banks keep what the file holds.

## Differences from the 0.7.0 driver

- One live machine per `Machine`: steps collect until a read, then run; steps after a read
  continue from where the machine is, nothing is replayed from power on.
- Each `key` and `type` is followed by a wait until the machine is idle, as a format-2 preset item
  is. `wait(n)` waits n seconds after that. So a `key` that starts a long run (an erase) returns
  when it has ended, and a following `expect` sees its end, not its progress.
- A suite's subclass may give `command()` a meaning of its own; the steps don't go through it.
- `lcd` gives all four rows, empty ones included.
- Module files need `format-version: 1` (required since 0.8.0).
- Serial port tests: `command("c1600 serial rs232c feed <file>")`, `… capture <file>`, `… report`.
- Every key can be pressed by name, the PC-1600's `bs`, `ctrl` and `kbii` too: `key("bs")`.

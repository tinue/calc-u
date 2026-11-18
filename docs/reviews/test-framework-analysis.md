# The PC-1600 test framework: analysis (2026-11-18)

Analysis only, no plan: what an outside tester needs to write pytest suites
against Calc-U-1600 with the driver `products/calc-u-1600/tools/machine/`, what
`main` (0.9.0) already gives, and what is missing. The user's rulings of
2026-11-18 are in. Paths are relative to the repository root; `1600/` means
`products/calc-u-1600/`.

## 1. Background

An outside tester wrote a pytest driver for Calc-U-1600 0.7.0 on `pc1600_cli`
(generated presets, one run from power on per read). The repository took it
over as `1600/tools/machine/` on `calcu serve` (LIB-11); the API of the test
cases stays, the inside may change. Their private suites (not in this
repository, not to be published) were written against 0.7.0 and will change
for 0.9.0.

On 0.7.0 the downloaded app was not enough: a test needed a source checkout,
the ROMs fetched into `roms/`, and a local build of `pc1600_cli` with
`tools/build_pc1600_cli.sh`, edited for `g++` and the Linux sharpdx library on
WSL. No release carried the CLI.

**Ruling:** build only, as today. Supported on macOS and Linux (WSL on
Windows), not on Windows itself. No release package. The ROMs are fetched into
the checkout before the build, so no ROM parameter is needed.

## 2. The wishes

The tester's wishes 1 to 5 (`REQUIREMENTS.md` of their 0.7.0 package) and 6
to 12 (later; 6 to 12 come from a new project, the superRAM suites need none
of them).

| # | Wish | Status on `main` |
|---|---|---|
| 1 | Read memory and ports; `dump:`, `expect-memory:`, `expect-port:` | Done: `peek(target, space, address, count, bank=, vbank=)`, `expect_memory`, `peek … file=`; ports through `io` and `io-written`. Not checked: a linear offset into a slot's card, and an `IN` read without its side effects |
| 2 | `expect-not:` | Done: `not_expect`, also a preset step (`not-expect:`) |
| 3 | The LCD dot matrix, 156 × 32 | Done: `display()` |
| 4 | Save and restore the full machine state | Done: `save_state`, `restore_state` |
| 5 | Open bus gives the last byte, not `&FF` | Deferred (user). `1600/TODO.md` |
| 6 | `--rom-dir`, paths independent of the working directory | Done: the ROMs are part of the checkout's build (`1600/app/HarnessProduct.cpp`); nothing depends on the working directory |
| 7 | `CXX` and the library name in the build script | Moot: the root CMake project builds `calcu`; the Linux CI image is Ubuntu 24.04, GCC 13.3 (`ci/linux/Containerfile`) |
| 8 | Empty `sampled-lines` or `bank-count: 1`, for a 32 KB mode of a banked card | Half: `bank-count: 1` is read; an empty `sampled-lines` is refused (`1600/core/Connector/MemoryCardDefinition.hpp`). Open question §5.2 |
| 9 | The built-in RS-232C, with preset steps that send to it and expect text from it | Part, §3 |
| 10 | The keys `bs` and `ctrl` in presets | Open. Refused on purpose (LIB-6 AC-44, `1600/app/SharpTyping.cpp`), with `kbii`. **Ruling:** an oversight; every key must be pressable by name, `bs`, `ctrl` and `kbii` included, in presets and in the harness |
| 11 | Save a card as a raw image, the counterpart of `encoding: file` | Open: `saveas` writes `.module.yaml` only (`hex`, `base64`, `addressed-hex`) |
| 12 | Run until the CPU reaches an address, or fail after a time | Done: `run_to(target, address, timeout)` (`c1600 call`, the debugger's `break`) |

## 3. The RS-232C (COM1)

What the tester needs: COM1 in the emulator, the PC-1600's own settings
(`SETCOM`, handshake), and data coupled on the other side, as the host drive
couples a folder (`S3:`).

What `main` has: `c1600 serial rs232c feed <file> [hold=<n>]` (the bytes one
per character time of the set speed), `capture <file>`, `report` (speed, data
bits, what was sent and taken) and `link` (a serial port file for a program
on the computer); in the driver through `command(...)`.

What it does not have:
- Handshake: the scripted peer reports CTS, DSR and DCD always on
  (`library/peripherals/SerialPort.hpp`); a test cannot drop them or watch
  RTS and DTR.
- A dialogue: the feed is a whole file at the start; there is no "wait until
  the PC-1600 has sent X, then answer Y".
- Preset steps for the port.
- `report` does not show parity, stop bits or the handshake set by `SETCOM`.
- Known gap: `SETDEV KI` with `INPUT` does not work on the PC-1600
  (`1600/help/09-com-ports.md`).

Handshake and a dialogue cannot be tested with the scripted peer; they would
need a new coupling, for example a pipe with an agreed protocol. Waiting for
the tester (§5.1).

## 4. The driver, the guide, the samples

**Driver** (`1600/tools/machine/`):
- A test brings its own media: a slot takes only a name (a file in `cards/`
  or a bundled module, `machine.py` `setup()`), not a path, and there is no
  floppy or cassette parameter. The harness has all of it (`slot-N: { file: … }`,
  `floppy { file: … }`, `cassette { file: … }`, `open <path>`), so it is a
  driver change. **Ruling:** a test must be able to bring its own card, disk
  and cassette files.
- The keys of wish 10; the README's `key-down bs` workaround goes.
- A method named `command` in a subclass hides `Session.command`, which
  `load()` uses; the README says so, or the driver's internal calls use a
  name of their own.
- The 0.7.0 semantics that changed (one live machine, no replay from power
  on; `key` and `type` wait until idle; `lcd` gives four rows) are in the
  README; suites written for 0.7.0 adapt to them.

**Guide:** `1600/tools/machine/README.md` grows into the guide, linked from
the help's "Without a window" sections (`1600/help/09-com-ports.md`,
`07-debugger.md`). Missing: the setup (clone, `1600/roms/fetch_roms.sh`, CMake
without Qt, the target `calcu`; on macOS and on Linux or WSL, with a
container file from `ci/linux/` for WSL users), a first test end to end, the
test's own media, the serial port and its limits, memory, display, state and
`run_to`. To check while writing it: whether a `calcu`-only Linux build needs
`fetch_sharpdx.sh`, and the lowest Python the driver takes.

**Samples**, our own only:
- The PC-1600 ROM dumper (repository `PC-1600-ROM`, `dumper/`): sets up COM1
  from BASIC, sends 16 KB pages with the serial IOCS on a key press; a test
  captures them and compares them with the ROM files. Works with `capture`
  today; it does not test a handshake.
- A BASIC program for `expect` and `not_expect`; a machine-code program for
  `peek` and `run_to`; a test with its own card file.
- The tester's programs and suites are not samples unless they agree.

## 5. Open questions (asked the tester, 2026-11-18)

1. RS-232C: is feed, capture and report with the lines always on enough, or
   are handshake lines (CTS/DSR set, RTS/DTR watched) and a dialogue needed?
   If so, a new coupling (§3).
2. The tester's new superRAM card has a jumper for Slot 1, where it must
   ignore the bank select. Jumper settings are separate card definitions, so
   the proposal is a Slot 1 definition without banking (one RAM region, Port
   `&28` ignored) instead of an empty `sampled-lines`. Asked: which 32 KB are
   seen in that setting, and does PVOUT still select the 16 KB halves?

# The harness with Calc-U-1600

`calcu --product c1600` runs the PC-1500, PC-1500A and PC-1600 without a
window: the product's app layer (`app/SharpAppPart`, as the frontends host
it; `app/HarnessProduct.cpp`) in emulated time only, so two runs give the
same replies. This file holds what is Calc-U-1600's: where the run finds
its files, the product's values of the common commands, and the `c1600`
commands (`app/SharpHarnessCommands.cpp`). The common harness (commands,
replies, `--json`, options, exit status, `serve`, `attach`) is in
`docs/formats/harness.md` at the repository root; the DAP server of
`serve --dap <port>` is in `DAP.md`.

## Running it

| What | How |
|---|---|
| Build | the target `calcu` of a root tree: `cmake --build ../../build/tests --target calcu`; the program is `build/tests/library/harness/cli/calcu` |
| One-shot | `calcu --product c1600 wait 2s then display-text then ticks` |
| Serve | `calcu --product c1600 serve`: one command per line, one reply per line; from Python `library/harness/python/calcu_serve.py` |
| Debugger | `calcu --product c1600 serve --dap <port>` (`DAP.md`) |
| Attach | a development or test build of the Qt6 app (`CALCU_ATTACH`) listens on `--attach-port N`, else 59059; `calcu attach [--port N] [--json] [--messages]` drives its session |
| pytest | `tools/machine/` (`Machine`, on `calcu serve`; its `README.md`) |
| Scratch output | `headless/` (`--output-folder headless`) |

The run finds, in the source tree:

| Files | Where |
|---|---|
| ROMs | `roms/`, through the product's ROM lookup (`sharp::lookUpRoms`) |
| Module definitions (`*.module.yaml`) | `frontends/qt6/resources/cards/`, and the ROM set's `CE-502B.module.yaml` |
| Host drive ROM | `firmware/pc1600-hostdrive/` |
| Bundled presets | `examples/` (`examples/bundle.yaml`; `examples:<path>`) |

It starts the startup model (the PC-1500A under the default settings),
cold booted, at key readiness. The host drive may be given a host folder
(`c1600 host-drive`); serial ports get a host link only on `c1600 serial …
link`.

## Product values of the common commands

| Command | Calc-U-1600's values |
|---|---|
| `model <m>` | `PC-1500`, `PC-1500A`, `PC-1600` |
| `variant <v>` | PC-1500: `A01`, `A03`, `A04` (default `A04`); PC-1500A: `A04`; PC-1600: `New`, `Old` (default `New`) |
| `attach <place> <peripheral>[:<variant>]` | places `slot-1` (`slot-2` on the PC-1600), `printer` (`CE-150`, `CE-1600P`), `interface` (`CE-158`), `host-drive` (PC-1600: `host-drive`), `bus-rom` (`bus-rom`, several in plug order); a module by its name (`CE-1600M`, …); `CE-1600P:new`, `CE-1600P:old` |
| `key <key ID> …` | the key IDs, any case; `break` for `on` |
| `type <text>` | the text and ENTER; replies when ENTER is up |
| `display-text` | the rows of text, the cell under the cursor read as the character the ROM saved there (`app/LcdTextPart`), then `indicators:` and `not text:`, or `display off` |
| `audio <file.wav>` | the piezo's sound into a WAV |
| `expect`, `not-expect` | text in one line of `display-text`, or `/regex/` |
| `screenshot <name>` | Copy Screen's picture, into `--output-folder` |
| debug targets | PC-1500, PC-1500A: `LH5801`; PC-1600: `SC7852`, `LH5803` |
| `peek`, `poke` spaces | `LH5801`, `LH5803`: `me0`, `me1`, `me1-written`; `SC7852`: `memory`, `io`, `io-written`; `bank=`, `vbank=`; `peek … file=<file>` writes the bytes to a file |
| `paper` | `paper marks\|png <path>\|copy\|cut [CE-150\|CE-1600P]`, `cut-paper` |
| media places | `slot-1`, `slot-2` (modules), `floppy` (CE-1600P, sides 0 and 1), `cassette-recorder` (CE-150, CE-1600P) |
| media kinds | `battery-backed-module` (`.module.yaml`), `floppy-disk` (`.floppy.yaml`), `cassette` (`.cassette.yaml` with its WAV); codecs in `app/SharpMediaKinds` |
| media steps | `floppy { name: Formatted, side: 1 }`, `cassette { file: x.cassette.yaml }`, `eject floppy\|cassette`, `saveas { place: slot-1, as: live, name: Mine }` |
| `open <path>` | presets `.pc1500`, `.pc1500a`, `.pc1600` (or by their `model:` line); media `.module.yaml`, `.floppy.yaml`, `.cassette.yaml`; a BASIC program (`basic-program`) or machine code (`machine-code-program`) loaded at once (a WAV's first file; headerless code is refused, it needs an address) — the kinds in `app/SharpFileKinds` |
| preset keys and sections | `program:`, `host-drive:`, `slot-N-rom:`, `bus-rom:`, `debug:` (`app/SharpPresetKeys`, rules `app/SharpPresetRules`) |
| `state-save`, `state-restore` | the whole machine with its plugged peripherals and their media: processors, all memory with its banks, chips and clocks, modules, the CE-150's and CE-1600P's mechanism, the CE-158, the disk with its side, the cassette with its sound and position, the host drive's open handles (not its folder). Not in it: what a media place is believed to hold, the `cassette record` path, the paper, the serial queues, breakpoints and history. The records: `core/Machine/StateRecords.hpp` |

## The `c1600` commands

Every command is offered on every model; one that needs what the model
lacks is refused with a sentence that says so. A command that needs a
machine is refused with the session's "no machine" sentence when there
is none. A file argument is a path relative to the working directory.

| Command | Does |
|---|---|
| `c1600 file-kind <file>` | what a program file is |
| `c1600 load-basic <file> [<n>]` | loads a BASIC program into memory |
| `c1600 type-basic <file>` | types a BASIC listing in through the ROM's editor |
| `c1600 load-code <file> [<n>] [address=<a>] [length=<l>] [cpu=<lh5803\|z80>] [run]` | loads machine code |
| `c1600 dump-basic` | the BASIC pointers and the program area |
| `c1600 list-basic` | the BASIC program as listing lines |
| `c1600 inspect <view>` | an INSPECTOR view as text |
| `c1600 cassette record <file.wav>` | a new cassette whose recording goes to the WAV |
| `c1600 cassette status` | the tape counter and the motor |
| `c1600 serial ce158\|rs232c feed <file> [hold=<n>]` | feeds a file into a serial port |
| `c1600 serial ce158\|rs232c capture <file>` | writes what the port sends into a file |
| `c1600 serial ce158\|rs232c link` | gives the port a host link |
| `c1600 serial ce158\|rs232c report` | the port's settings and traffic |
| `c1600 host-drive <folder>\|off` | the PC-1600's host drive with a host folder, or detached |
| `c1600 call <target> <address> [timeout=<duration>]` | runs a routine on a paused machine until it returns |

`calcu --help` lists them.

### Program files

#### The classifier

`c1600 file-kind` and every loader ask one classifier
(`programfile::classify`, `core/ProgramFile.cpp`; `sharp::loaders::fileKind`,
`core/Loaders.cpp`). It looks at the content, never at the name.

| Kind | The file |
|---|---|
| `empty` | no bytes |
| `basic-listing` | an ASCII BASIC listing |
| `basic-pc1500` | tokenized BASIC behind a CE-158 header |
| `basic-pc1600` | tokenized BASIC behind a PC-1600 header |
| `code-lh5801` | machine code behind a CE-158 header |
| `code-z80` | machine code behind a PC-1600 header |
| `headerless` | any other binary: machine code without a header |
| `other` | a Reserve Area, variables, plain text, or a cassette WAV without a file that can be read |

A cassette WAV is decoded and described as its first readable file; its
readable files are listed in tape order.

Header fields (for the four headered kinds):

| Field | Meaning |
|---|---|
| name | the file name the header carries; may be empty |
| load address | code only: 16 bits; `code-z80`: the bank in bits 16–23 of the header's address |
| bank | `code-z80` only; 0 otherwise |
| length | the payload length the header gives |
| start | code only: the auto-run address; a header with 0 or FFFF has none |

Words of an `other` file: `a Reserve Area (CE-158 header)`, `a Reserve
Area as text`, `variables (CE-158 header)`, `variables as text`, `plain
text`; a WAV without a readable file: `a cassette recording without a file
that can be read (<why>)`.

`damaged`: a header's start without a complete header of a known kind, or
a payload that does not decode.

#### `c1600 file-kind <file>`

Reply lines, in this order (each only where it applies):

| Line | When |
|---|---|
| `kind: <kind>` | always |
| `name: <name>` | a headered kind whose header carries a name |
| `load: &<hex>` | `code-lh5801`, `code-z80` |
| `bank: <n>` | a header with a bank other than 0 |
| `length: <n>` | a headered kind |
| `start: &<hex>` or `start: none` | `code-lh5801`, `code-z80` |
| `looks-like: lh5803\|z80` | `headerless` bytes that look like LH5801 or Z-80 code (a guess; it never chooses the processor) |
| `damaged: yes` | damaged |
| `words: <words>` | `other` |
| `file <i>: <name>` | a WAV: one line per readable file, from 1; a file without a name is `(no name)` |

`--json`: `{"kind", "name"?, "load"?, "bank"?, "length"?, "start"?,
"looksLike"?, "words"?, "files"?}`. `name`, `load`, `bank`, `length` and
`start` come together for a headered kind (`load`, `start` numbers, 0
where none); `files` is the array of the WAV's file names. `damaged` is not
in the JSON.

Refusals: `usage: c1600 file-kind <file>`; `The file <file> cannot be
read.`; the busy sentence while an operation runs.

#### `c1600 load-basic <file> [<n>]`

`<n>` (a decimal number) chooses the file of a cassette WAV, from 1;
without it the first readable file. A listing or tokenized BASIC is put
into the program area at once (not typed). The reply is `done`, or
`refused: <file>: <sentence>`. The sentences include:

- `The file is empty.`
- `This file holds <words>, not a BASIC program.`
- `This is neither a BASIC listing nor tokenized BASIC with a CE-158 or PC-1600 header.`
- `This is a PC-1600 tokenized BASIC program -- load it on a PC-1600.`
- `The cassette recording holds <k> file(s).`; `The file is no cassette recording with <n> files.`
- on a PC-1600 in MODE 1 a listing is read as PC-1500 BASIC, and a refusal says so.

Other refusals: `usage: c1600 load-basic <file> [<n>]`, `unknown argument
<a>`, `The file <file> cannot be read.`, the busy sentence.

#### `c1600 type-basic <file>`

Types the listing in, line by line, through the ROM's editor, as an
operation; the machine must be in program mode (PRO) first. Only a listing
(`basic-listing`, or `other` with the words `plain text`) is typed; a line
without a number is typed too and then counts as not stored.

| Reply | When |
|---|---|
| `done` | every line was stored |
| `refused: <file>: The file is empty.` | an empty file |
| `refused: <file>: Only a BASIC listing (text) can be typed in.` | any other kind |
| `refused: <file>: <N> program line(s) not stored (too long, or rejected by the editor): "<line>", "<line>".` | some lines were not stored |
| `refused: <file>: No program line was stored -- is the machine in PRO mode? (…): "<line>", ….` | none was stored |

Other refusals: `usage: c1600 type-basic <file>`, `The file <file> cannot be
read.`, the busy sentence.

#### `c1600 load-code <file> [<n>] [address=<a>] [length=<l>] [cpu=<lh5803|z80>] [run]`

| Argument | Meaning |
|---|---|
| `<n>` | the file of a cassette WAV, from 1; else the first |
| `address=<a>` | the load address, hex: `C000`, `&C000`, `$C000`, `0xC000`. A headerless file needs one |
| `length=<l>` | the number of bytes (decimal), over the header's length |
| `cpu=lh5803\|z80` | PC-1600: the processor whose address space `address=` is in (any case) |
| `run` | after the load, type the starting command and wait until the machine is idle (the prompt ready for 20 frames in a row; at most an hour of emulated time, then done all the same with a log line). Only code whose header names a start address is started |

Reply lines, in this order:

| Line | Meaning |
|---|---|
| `range: &<first>-&<last>` | the bytes written, in the processor's address space |
| `area: S0\|S1\|S2` | PC-1600 only: the program area |
| `bank: <n>` | PC-1600 only: the bank |
| `new: <command>` or `new: none` | the `NEW` that keeps BASIC off the code (`NEW &<a>`, `NEW "S<n>:",&<a>`) |
| `new-note: <sentence>` | why that `NEW`, or why none |
| `start: <command>` | the starting command: `CALL &<a>`, `CALL #<bank>,&<a>`, `XCALL &<a>` |
| `start-note: <sentence>` | where the start address comes from |
| `warning: <sentence>` | one per warning (code in a work area BASIC can overwrite) |
| `started: yes\|no` | whether `run` started it |

`--json`: `{"range", "area"?, "bank"?, "new", "newNote", "start",
"startNote", "warnings": [], "started"}` (`area` and `bank` on the PC-1600
only; `new` is empty where the line says `none`).

Refusals: `refused: <file>: <sentence>` from the loader, among them `Not a
hex address (&0000-&FFFF).`, `There is no processor "<cpu>" (lh5803,
z80).`, the classifier's sentences for a file that is no machine code
(`This is a BASIC listing, not machine code -- use Load BASIC Program.`, …);
the command's own: `usage: c1600 load-code <file> [<n>] [address=<a>]
[length=<l>] [cpu=<lh5803|z80>] [run]`, `unknown argument <a>`, `length=
takes a number of bytes`, `The file <file> cannot be read.`; the busy
sentence.

The load dialog's plan (`c1600.code-plan`: the proposed address, the
processor and its note, where the code would go) is a session request; the
harness has no command for it.

### `c1600 dump-basic`

The BASIC pointers, one per line, then the program area from its start to
16 bytes past its end, 16 bytes a row. No arguments.

PC-1500, PC-1500A:

```
BASPRG_ST  $7865 = $40C5
BASPRG_END $7867 = $40D2   (len = 13)
BASPRG_EDT $7869 = $40C5
VAR_START  $7899 = $....
DATA_PTR   $78BE = $....
RAM_END    $7864 = $.. (page)   WARM_START $7A20 = $..
40C5: 00 0A 09 F1 97 ...  |.....|
```

PC-1600 (the LH5803's addresses, with the Z-80's beside the start and
end; Z-80 = LH5803 + $8000 below $8000):

```
BASPRG_ST  $F865 = $40C5  (Z-80 $C0C5)
BASPRG_END $F867 = $40D2  (Z-80 $C0D2, len = 13)
BASPRG_EDT $F869 = $....
F5CF (LE, RAM base) = $....   F89D (LE) = $....
VARIABLE_PTR $F899 = $....
C0C5: 00 0A ... |....|
```

A row is `<address>: ` and 16 bytes in hex, then the bytes as characters
between `|` (printable ASCII as it is, any other byte `.`). The rows are in
the main processor's view (PC-1600: the Z-80's). When the start does not
lie before the end, the rows are left out and the pointers stand as they
are.

`--json`: `{"pointers": [{"name", "address", "value", "z80"?, "length"?}],
"rows": [{"address", "bytes": []}]}`. Pointer names: PC-1500, PC-1500A
`BASPRG_ST`, `BASPRG_END`, `BASPRG_EDT`, `VAR_START`, `DATA_PTR`,
`RAM_END`, `WARM_START`; PC-1600 `BASPRG_ST`, `BASPRG_END`, `BASPRG_EDT`,
`F5CF`, `F89D`, `VARIABLE_PTR`. `z80` is on the PC-1600's `BASPRG_ST` and
`BASPRG_END`, `length` on `BASPRG_END`.

Refusals: `usage: c1600 dump-basic` (an argument given); `The machine has
no BASIC memory.`

### `c1600 list-basic`

The program as listing lines, one per program line, as the detokenizer
(`libsharpdx`, `sde_detokenize`) writes them, with the model's keyword
table: no blank between a variable or number and the keyword after it
(`40 ON N+1GOTO 70,50`). PC-1500, PC-1500A: the program from BASPRG_ST
($7865) to its FFH end mark. PC-1600: the program area TITLE selects (S0,
or the S1/S2 program module), read across its module banks and internal
RAM as the BASIC loader lays it down: a module bank's part ends with its
00 00 mark. No program: `done` and no lines.

`--json`: `["10 PRINT 1", "20 GOTO 10"]`.

Refusals: `usage: c1600 list-basic` (an argument given); `The machine has
no BASIC memory.`; `list-basic reads a program in MODE 0 only.` (a PC-1600
in MODE 1); the placement's reason when the program area can't be worked
out; `The program can't be read: <the detokenizer's reason>`.

### `c1600 inspect <view>`

The view of the INSPECTOR, one reply line per line of the view.

| View | Shows |
|---|---|
| `pointers` | Pointers |
| `inventory` | Inventory |
| `lh` | LH5801 View (PC-1500) / LH5803 View (PC-1600) |
| `z80` | Z80 View |
| `basic` | BASIC Area |
| `programs` | Program Areas |
| `disks` | Disk Areas |
| `physical` | Physical RAM |
| `dump-basic` | the BASIC area as a dump |
| `dump-ml` | the Machine-Language Area as a dump |
| `dump-s1`, `dump-s2` | Program Area S1, S2 as a dump |
| `dump-disk-s1`, `dump-disk-s2` | Disk S1:, S2: as a dump |

A view the model lacks is a done reply of one line, `<title>: not on the
PC-1500` or `<title>: not on the PC-1600`.

`--json`: the array of the lines.

Refusals: `usage: c1600 inspect <view>`; `No view <name>. The views:
pointers, inventory, z80, lh, basic, programs, disks, physical,
dump-basic, dump-s1, dump-s2, dump-disk-s1, dump-disk-s2, dump-ml.`; `The
machine has no INSPECTOR.`

### `c1600 cassette record <file.wav>`, `c1600 cassette status`

| Command | Reply |
|---|---|
| `record <file.wav>` | `done`. A tape in the cassette recorder is ejected first (written as the media part writes it); a new, empty tape goes in, bound to the WAV. The tape's sound is written to the WAV each time the motor stops and at `eject`; no cassette definition is written. A path that cannot be written fails at that write with a notice (`state notices`); the command itself is done |
| `status` | `cassette: playing <position> of <length>`, `cassette: recording <length>` or `cassette: empty`; then `motor: on\|off`. Seconds with nine decimals |

`status --json`: `{"cassette": "playing|recording|empty", "position",
"length", "motor"}`; `position` and `length` are the seconds as text with
nine decimals, `motor` a boolean.

Refusals: `usage: c1600 cassette record <file.wav> | cassette status`;
`The machine has no cassette: neither a CE-150 nor a CE-1600P is plugged
in.`; `record` through `attach`: `This command needs a run of calcu; it is
not offered through attach.`

### `c1600 serial ce158|rs232c …`

`ce158` is the CE-158's port, `rs232c` the PC-1600's own. The port is the
running machine's; a port it does not offer is refused, and nothing is
kept for a machine built later. No port has a host link unless `link`
asks.

| Sub-command | Does | Reply |
|---|---|---|
| `feed <file> [hold=<n>]` | Offers the file's bytes to the port, one in each character time (ten bits at the speed the port is set to; 300 baud while it has none), counted in emulated ticks, after `n` character times of nothing. A second `feed` replaces what was left of the first | `done` |
| `capture <file>` | Makes the file empty, then appends the bytes the port sends. It is complete at `quit`, at the end of the run, and when another `capture` names another file | `done` |
| `link` | Gives the port its host link: a serial port file (`calcu1600-ce158.serial`, `calcu1600-rs232c.serial`) in the run's serial-link folder | `path: <path>`; `--json` `{"path"}` |
| `report` | The port's settings and what went through it | the lines below |

`report` lines:

```
speed: <baud> baud | not set
bits: <n> | not set
used: yes|no
host link: yes|no
received: <taken> of <fed> bytes fed
sent: <n> bytes
| <text line>
printer: <n> bytes          (ce158 only: the CE-158's printer port)
| <text line>
```

`sent` holds every byte the port sent since the machine was built; under
`sent:` and `printer:` the bytes follow as text, one `| ` line per line.

Bytes as text: a line end (LF) and printable ASCII (20–7E) as they are, a
carriage return left out, any other byte `<XX>` in hex
(`core/Connector/PrintableBytes.hpp`; the CE-158's printer pane uses the
same rule).

`report --json`: `{"baud", "used", "taken", "fed", "sent", "printer"?}`;
`baud` 0 when not set, `sent` and `printer` the text.

Refusals: `usage: c1600 serial ce158|rs232c feed <file> [hold=<n>] |
capture <file> | link | report`; `The machine has no CE-158.`; `The
machine has no RS-232C port: only the PC-1600 has one.`; `hold= takes a
number of character times.`; `The port has a host link; feed and capture
are refused for it.`; `The file <file> cannot be read.`; for `capture` the
reason the file cannot be written; `This host has no pseudo-terminals: the
port gets no host link.`; through `attach`: `This command needs a run of
calcu; it is not offered through attach.`

### `c1600 host-drive <folder>|off`

On a PC-1600, attaches the host drive with that folder (the whole rest of
the line is the folder, made absolute), or, when it is attached already,
gives it the new folder; `off` detaches it (`done` when none is attached).
The machine is changed as by `attach` and `detach`.

Refusals: `usage: c1600 host-drive <folder> | off`; `The host drive is a
PC-1600 peripheral.`; `The path <path> is no folder.`; the plug check's
sentence; the busy sentence.

### `c1600 call <target> <address> [timeout=<duration>]`

The machine must be paused. `<target>` is a debug target (any case);
`<address>` a number of 16 bits (`&`, `$` or `0x` for hex, else decimal).
The target's processor makes the call from the current state as its own
call instruction does (the return saved, the program counter set to the
address); then the machine runs until the routine has returned (the
debugger's step-out), at most `timeout` of emulated time (default `10s`;
a duration with its unit, `2s`, `500ms`). Past the timeout the machine is
paused.

Reply lines:

```
returned: yes | no, timeout
<register> = 0x<hex>        (one per register of the target)
ticks: <n>
seconds: <s>
```

`--json`: `{"returned", "registers": {"<name>": "0x<hex>"}, "ticks",
"seconds"}`.

Refusals: `usage: c1600 call <target> <address> [timeout=<duration>]`;
`timeout= takes a duration: <why>`; `The address <a> is no address of 16
bits.`; `The machine is not paused (pause first).`; `The machine has no
debug target <target>.`

## Pitfalls

- `key` and `type` do not wait until the machine is idle; `wait` without a
  duration does.
- `type-basic` needs program mode (PRO) first; in RUN mode no line is
  stored.
- Headerless machine code needs `address=`; `open` refuses it.
- `serial … feed` and `capture` are refused on a port with a host link.
- The save folder of a run is empty and removed at the end unless
  `--save-folder` names one.

# Preset format

A preset is a file that sets up a machine: the model, its peripherals and
modules, then program, data and scripted input. One format serves both
products. This document specifies `format-version: 2`.

The rules all formats share (syntax, byte blocks, version, name,
addressing a resource, errors, the markers [both], [59], [1600],
[undefined]) are in [README.md](README.md).

## 1. The file

- **Suffix:** `.ti59`, `.ti58`, `.ti58c`, `.pc1500`, `.pc1500a` or
  `.pc1600`. The suffix doesn't choose the model; `model:` does.
- **Contents:** top-level keys of three sorts:
  - **Header keys** describe the preset or its machine. They act on the
    whole file wherever they stand, and each appears at most once.
  - **Sections** are carried out in file order, as often as they appear.
  - **Steps** are the items of a `keys:` section (`- verb: value`).

```yaml
format-version: 2
model: TI-59
preset-name: Pgm 01
description: Master Library diagnostic, run at regular speed
slot-1: { name: ML }
printer: PC-100C
keys:
  - regular-speed:
  - key: Pgm 0 1 SBR =
```

## 2. What a load does

1. **Build the machine** of the model and its variant, with the
   peripherals, modules and media the header keys name, and nothing else.
   A hardware key that is absent means **none**, on every model. Every
   keyword a preset leaves out has a default, given where it is
   specified; where none is given, the cold boot's state holds.
2. **Cold boot** it, as on the real hardware: all memory zeroed, the
   constant memory of a TI-58C included, before the processor is
   powered; then the ROM's start-up runs.
3. **Carry out the sections** in file order (§4). Each item starts only
   after the item before it has finished, unless a `skip-wait:` follows
   that item ([§6](#6-speed-and-waiting)).
4. **End** once the last item has been carried out; it is not waited
   for, unless it is `let-it-finish:`. The machine runs on at regular
   speed, under the user's control.

Without a `cue-card:` section, no cue card is fitted.

## 3. Header keys

| Key | Where | Value |
|---|---|---|
| `format-version` | [both] | `2`. Required |
| `model` | [both] | Required. `TI-59`, `TI-58`, `TI-58C` [59]; `PC-1500[:A01\|A03\|A04]` (default `A04`), `PC-1500A` (ROM A04 only), `PC-1600[:new\|old]` (default `new`) [1600]. Any case. The part after `:` is the **variant** |
| `preset-name` | [both] | The name tag. Optional; default: the file stem |
| `description` | [both] | One line for lists, menus and tooltips |
| `timeout` | [both] | The longest the preset waits between two items for the machine to become idle, in seconds of emulated time ([§6.3](#63-timeout)). Optional; default `3600` |
| `printer` | [both] | `PC-100C` [59]; `CE-150` [1600]; `CE-1600P[:new\|old]` (default `new`) [1600: PC-1600]. Any case |
| `interface` | [1600] | `CE-158` |
| `slot-1` | [both] | A module, as a reference by `file:` or `name:`. [59]: a Solid State ROM module ([modules.md §2](modules.md#2-solid-state-rom-module)); `name:` is its catalogue ID. [1600]: a module definition ([modules.md §1](modules.md#1-module-definition)); `name:` is its `module-name` |
| `slot-2` | [1600: PC-1600] | As `slot-1` |
| `slot-1-rom`, `slot-2-rom` | [1600] | A ROM file, by `file:` only, whose bytes replace the ROM of the module in that slot (§3.2) |
| `host-drive` | [1600: PC-1600] | The folder that backs drive `S3:`, by `folder:` |
| `bus-rom` | [1600] | ROMs on the 60-pin connector (§3.3) |
| `debug` | [1600] | The debugger configuration of a project preset (§3.4) |
| `magnetic-cards` | [59: TI-59] | Magnetic cards defined inside the preset (§3.1) |

```yaml
format-version: 2
model: PC-1600:new
printer: CE-1600P
slot-1: { name: CE-1600M }
slot-2: { file: cards/my.module.yaml }
```

The header holds what is installed before the machine starts: the
printer, the interface, the modules, the ROMs, the host drive.
Removable media, which are changed while the machine runs (a magnetic
card, a floppy disk, a cassette), are steps (§5.2).

### 3.1 `magnetic-cards` [59: TI-59]

A list of **in-file cards**: magnetic cards defined by the preset, which
a `card:` or `arm:` step addresses with `mem: <card-name>`. A bank
written to it is kept, and read back by its next use, until the next
load or model switch; a reset, a power cycle or a cold boot keeps it,
as a real card keeps what was written. An in-file card is saved to a file only by
`saveas:` (§5.2.1), with its cue card.

```yaml
magnetic-cards:
  - card-name: MyVars
    cue-card:                 # optional
      title: My Vars
    sides:
      0:
        bank: 3               # required, 1–4
        protected: false      # optional, default false; true = written as −n Write
        partition: 479.59     # optional; default: the machine's when the card goes in
        data:                 # registers of bank 3: R30–R59
          30: 1.5
          31: 2.25
        program: |            # keycodes; bare keycodes start at the bank's first step (480)
          76 11 42 00
```

- Each item has `card-name`, optional `description`, optional
  `cue-card` ([cue-card.md](cue-card.md)), and
  `sides` with `0`, `1` or both, or `{}` for a fresh card. A side not
  given is fresh (unwritten).
- **A side replaces its whole bank.** Steps and registers it does not
  list are read as 00. Bank n holds program steps 240·(n−1) … +239 and
  data registers 120 − 30·n … 149 − 30·n (bank 4 = R00–R29, bank 3 =
  R30–R59, bank 2 = R60–R89, bank 1 = R90–R99). Contents outside the bank
  are an error.
- `program:` uses the keycode notation of §4.1, `data:` the notation of
  §4.2.
- A side is built into the bytes the ROM would write, header and checksum
  included.
- A `mem:` reference to a card name the preset doesn't define is a
  **fresh** in-file card, without a cue card. The fresh card is the
  magnetic card's one template; it is always there and is not named in a
  preset ([media.md §1.2](media.md#12-card-files-and-in-file-cards)).
- A card's corners show the banks its sides hold; they are empty while a
  side is fresh and show the bank once it is written. They are not set
  by hand.
- Two items with one `card-name` are an error.

### 3.2 `slot-N-rom` [1600]

While a ROM module is being developed, its bytes come from the build:

```yaml
slot-1: { file: module.module.yaml }   # decode, banking, the ROM's size
slot-1-rom: { file: build/module.bin } # what the ROM holds
```

- It needs a module in the same slot.
- The module is a ROM module of one region (`content: rom`, no `by-bank`
  split).
- The file is exactly the region's size.
- The file is read each time the preset is applied.
- Such a module is never saved: no autosave, no Name & Save, and a
  `saveas:` of that slot fails.

### 3.3 `bus-rom` [1600]

A list of ROM files on the 60-pin connector, for developing a ROM
extension. A bus ROM shadows a built-in ROM at the same place.

```yaml
bus-rom:
  - file: build/ext.bin
    bank: 7          # PC-1600 system bus, page B bank 4–7
  - file: ext1500.bin
    address: &8000   # PC-1500 connector, or the PC-1600's LH5803 side
    me: 1            # optional: ME1 (default ME0)
    pv: 1            # optional: only while PV is 0 / 1
    pu: 0            # optional: only while PU is 0 / 1
```

Each item has `file` and either `bank` (4–7, PC-1600 only) or `address`
(&0000–&FFFF). `me`, `pv` and `pu` (0 or 1) go with `address` only.
`file` is a path, resolved as a `file:` reference is.

### 3.4 `debug` [1600]

What a debug session loads, builds and starts on the machine this preset
sets up. A loader that loads the preset for use, not for debugging,
ignores the block. The machine of the session is the one this preset
sets up; a `preset` key belongs to a launch configuration without a
project, not to the block. Its keys:

| Key | Meaning |
|---|---|
| `program` | The program to load: `bin`, `listing`, `source` (if the main source isn't `<listing>.asm`), `symbols`, `cpu` (`lh5801` / `z80` / `lh5803`), `address`, `entry`, `after` (`stopOnEntry` / `call` / `none`; default `none`), `cleanStart` (default `true`), and the bank qualifiers `bank`, `me`, `pu`, `pv` |
| `listings` | Listings of code that isn't loaded (ROM): a path, or `{path, source, cpu, bank, me, pu, pv}` |
| `symbols` | Symbol tables: a path, or `{path, cpu, bank, me, pu, pv}` |
| `command` | A BASIC line typed to start the code: in place of the program's `CALL`, or, without a program, once the machine is set up |
| `boot` | `debug`: run the power-on under the debugger (the preset's hardware only; no `program` or `command`) |
| `reset` | `none` (default) / `reset` / `allReset`: a Reset or a Cold boot that stops at the reset vector; the start-up runs from there under the debugger |
| `stopOnEntry` | Stop right after attaching (after a reset: before the first instruction) |

Numbers (`address`, `entry`, `bank`, `me`, `pu`, `pv`) follow the
number rule; an `entry` may also be a symbol.

## 4. Sections

Sections are carried out in file order and may appear any number of
times. A program, data, partition or cue card section after a `keys:`
section changes the machine in the middle of the script.

| Section | Where | Value |
|---|---|---|
| `program` | [both] | A program to put into memory (§4.1) |
| `data` | [59]; [undefined] on the Sharp models | Data to put into memory (§4.2) |
| `partition` | [59] | The partition, as a value (§4.3) |
| `cue-card` | [59] | A stand-alone cue card to fit (§4.4) |
| `keys` | [both] | A list of steps (§5) |

### 4.1 `program`

**[59]: a keystroke program.** One of:

- `keycodes: |`: the program inline;
- `file: <path>`: a keystroke program file (§4.1.1).

```yaml
program:
  keycodes: |
    76 11 42 29 92 76 12 42 30 92   # compact: 000-009
    010 43 RCL                      # listing: step number, keycode, mnemonic
    011 00 00
    # ...                           # a gap is a comment, if wanted
    110 42 STO                      # the step number places the keycode
```

Each line is in one of two notations:

- **Compact:** a line of keycodes, as many as wanted, each exactly two
  digits (`00` to `99`), like a hex listing of a ROM but with keycodes.
  The line goes on at the step after the one written last (step 000 at
  the start). Steps of `00` are written out; there is no gap marker.
  Any other token, a one-digit number or a mnemonic, is an error.
- **Listing:** as a PC-100C lists a program. A first token of 3 or more
  digits is a step number (0 up to the model's last physical step: 959
  on the TI-59, 479 on the TI-58/58C), which places the keycode; the
  next token is the one keycode, two digits; anything after it is the
  mnemonic, which is not read, even a number (`473 00 0`).
- A gap in a listing needs no marker, since the step numbers place the
  keycodes; a comment (`# ...`) can show it for the reader.
- **Only the listed steps are written.** Every other step keeps its
  contents (00 after the cold boot).
- Each `program:` section starts at step 000.
- Keycodes are not key IDs: digit 1 is keycode `01`, and `Pgm` is the
  one step `36`. Key IDs are for `key:` steps (§8).

#### 4.1.1 Keystroke program file [59]

A keystroke program kept in a file of its own, suffix `.keystrokes.yaml`:

```yaml
format: ti-keystroke-program
format-version: 1
program-name: Loan
description: Monthly payment of a loan
keycodes: |
  000 76 LBL
  001 11 A
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `ti-keystroke-program` |
| `format-version` | yes | `1` |
| `program-name` | yes | The name tag |
| `description` | no | One line about the program |
| `keycodes` | yes | The program, in the keycode notation above |

**[1600]: a BASIC or machine-code program.**

| Key | Value |
|---|---|
| `file: <path>` | Loaded by what the file holds: a BASIC listing (`.bas`), tokenized BASIC, machine code with or without a header, or the sound of a tape (`.wav`), whose program goes into the program area. A cassette definition is not a program file |
| `typed: true` | With a listing in `file:`: type it in line by line instead of writing it into memory |
| `address: <n>` | Machine code: where it goes. Required for headerless code |
| `length: <n>` | Machine code: load only this many bytes |
| `text: \|` | A BASIC listing inline, typed in line by line |

- Exactly one of `file:` and `text:`.
- `address`, `length` and `typed` go with `file:` only; `address` and
  `length` with machine code only; `typed` with a listing only.
- **PC-1600:** code and BASIC are placed by the machine's MODE, the
  program area `TITLE` selects, and the address. A preset switches MODE
  or `TITLE` in a `keys:` section before the `program:`.
- **Auto-run:** machine code whose header has an auto-run address is
  started with `CALL`.

### 4.2 `data` [59]

The data registers, as `NN: value`:

```yaml
data:
  00: 3.141592653589793
  05: -1.5e-3
  H01: 7.77E22           # TI-58C only
```

- `NN` = 00–99, the `STO`/`RCL` register number. Registers 60–99 exist
  on the TI-59 only; on the TI-58/58C they are an error.
- `HNN` = H00–H03: the TI-58C's four extra RAM cells beyond the 60
  registers, reached only by `MEMWR`/`MEMRD`. They are not registers
  60–63. They are an error on any other model.
- The value is a decimal literal; an exponent is allowed. Integers are
  stored digit for digit, other values rounded to 13 significant digits.
  A whole number with more than 13 significant digits keeps its first
  13: the digits after the 13th are cut.
- **Only the listed registers are written.** Every other register keeps
  its contents (zero after the cold boot).
- `program:` and `data:` are written as given, in file order; a listing
  may put registers in the program area or steps in the data area.
- **On the Sharp models**, the counterpart is pre-populated BASIC
  variables. It is **[undefined]**.

### 4.3 `partition` [59]

```yaml
partition: 479.59
```

- The number before the dot is the last program step; steps = number + 1.
  The part after the dot (the register count) is a note and is ignored.
- Steps are rounded up to a multiple of 80 (10 program registers), at
  most 960.
- A value above the model's last step (479 on the TI-58/58C) is an error.
- It takes effect where it stands in file order. Until then the
  partition is the model's after the cold boot: 479 (480 steps, 60 data
  registers) on the TI-59, 239 on the TI-58/58C.

### 4.4 `cue-card` [59]

Fits a stand-alone cue card ([cue-card.md](cue-card.md)) in the window
above the top row of keys, as the machine's own card ([cue-card.md §2](cue-card.md#2-fitting)). It doesn't go
onto magnetic cards: a magnetic card has its own `cue-card` (§3.1).

## 5. Steps

A `keys:` section holds a list of steps, each `- verb: value` or, for a
verb without a value, `- verb:`. **Every step starts only after the step
before has finished**, unless a `skip-wait:` stands between them
([§6](#6-speed-and-waiting)).

```yaml
keys:
  - key: Pgm 0 1               # 2nd LRN 0 1
  - key: SBR =
  - card: { mem: MyVars }
  - expect: 3.141592654
```

### 5.1 Input

| Step | Where | Meaning |
|---|---|---|
| `key: <id> [<id> …]` | [both] | **Press**: press and release each key, one after the other. Each press waits for key readiness before it goes down, except the first press of a step right after a `skip-wait:`, which goes down at once, ready or not; each press holds the key long enough for the machine to register it, but not much longer; the exact time is the product's. Key IDs (§8) are separated by spaces and compared in any case |
| `type: <text>` | [1600] | **Typing**: type the text as key presses, then ENTER. The text runs verbatim to the end of the line: `#` and quotes are typed, not read as a comment or quotes |

- `key:` presses keys; it doesn't type text. `key: PRINT` is an error,
  not five letters; text is typed with `type:`. Several keys on one line
  are still separate presses.
- A word that is not a key ID is an error. On the TI models a two-digit
  number such as `12` is an error; `1 2` is two digit keys.

### 5.2 Media

| Step | Where | Meaning |
|---|---|---|
| `card: <reference>` | [59: TI-59] | **Insert** a magnetic card: push one side through the card reader. Reference by `file:`, `name:` or `mem:`, with `side: 0\|1` (default 0) |
| `arm: <reference>` | [59: TI-59] | **Arm** a magnetic card: make it ready for the user to feed. Reference by `file:`, `name:` or `mem:`, without a side |
| `floppy: <reference>` | [1600: PC-1600 with `printer: CE-1600P`] | **Insert** a floppy disk ([media.md §2](media.md#2-floppy-disk)) into the CE-1600F drive, by `file:` or `name:`, with `side: 0\|1` (default `0`). A disk that is in the drive is ejected first |
| `cassette: <reference>` | [1600: with `printer: CE-150` or `CE-1600P`] | **Insert** a cassette ([media.md §3](media.md#3-cassette)) into the printer's cassette recorder, by `file:` or `name:`. A cassette that is in is ejected first |
| `eject: <place>` | [1600] | **Eject** the floppy disk (`floppy`) or the cassette (`cassette`): the place is empty afterwards, and its selector shows it so. An empty place: nothing happens. The place is named by its place word (the glossary's **Place word**): the kind's eject word where one place holds the kind, so `cassette` for the place `cassette-recorder` |
| `saveas: <target>` | [both] | **Name & Save** the medium or module at a place, or an in-file magnetic card [59: TI-59] (§5.2.1) |

**`card:`** is the user pushing a card into the calculator. The ROM
reads the side, or writes it if the machine waits for a card to write.
The step has finished when the card has passed. **After it has passed,
the card is fitted** ([cue-card.md §2](cue-card.md#2-fitting)).

- The TI-59 takes a card when it is idle, and when it explicitly waits
  for one (after `Write`). A machine waiting for a card counts as idle
  ([§6.2](#62-what-finished-means)), so `- key: 3 2nd Write` finishes
  when the machine waits for the card, and the next step, `card:`, feeds
  it. A card inserted while the machine can't take one is the ROM's
  matter, as on the real calculator.
- The ROM checks what it checks on real hardware: the partition on a
  read at idle, the bank number on `n INV Write`. Choosing the right
  card is the preset's job.
- A `file:` or `name:` reference to a card that doesn't exist is an
  error, as for every medium. A new card is an in-file card (§3.1),
  saved with `saveas:`.

**`arm:`** makes the card ready for the user to feed by hand: the card
is fitted, and the user feeds it by tapping a corner. Arming does nothing
electrically (no card is detected by the machine) and doesn't read the
card, so no side is chosen. A preset inserts cards with `card:`; it arms
one only to leave it to the user, typically as its last step.

#### 5.2.1 `saveas`

```yaml
- saveas: { place: slot-1, as: live, name: My card }
- saveas: { place: slot-1, as: live, protected: true, name: My ROM }
- saveas: { place: floppy, as: template, file: disks/Work.floppy.yaml }
- saveas: { card: PDCM-1, as: live, name: PDCM 1 }        # TI-59
```

| Key | Value |
|---|---|
| `place` | [1600]: the place word of the place whose medium or module is saved: `slot-1`, `slot-2` [PC-1600], `floppy` [PC-1600] or `cassette` (the place `cassette-recorder`) |
| `card` | [59: TI-59]: the `card-name` of an in-file card (§3.1), defined or fresh. Exactly one of `place` and `card` |
| `as` | `live`: an instance, saved again after each change. `template`: a template, never written again |
| `protected` | `true`: save with the module's write-protect switch on, and switch it on in the machine. Only for a module that has the switch (CE-161); not for a floppy disk or a magnetic card |
| `name` / `file` | Exactly one. `name`: the name tag of the saved copy, which goes into the save folder. `file`: the file to write; its name tag is the file name without the suffix (`.module.yaml`, `.floppy.yaml`, `.cassette.yaml`, `.magnetcard.yaml`), which it must have |

- A name must not contain `"`. A saved copy never takes the name of a
  bundled template.
- A module with a `slot-N-rom:` can't be saved.
- **A magnetic card** [59: TI-59] is saved with its sides as they stand
  (written by the machine or defined in §3.1) and with the in-file
  card's `cue-card`; its `card-name` becomes the name tag. Saved as
  `live`, the card is that instance from then on: a side written to it
  later is saved into the file. A typical preset defines the card with
  its cue card, writes it, then saves it:

```yaml
magnetic-cards:
  - card-name: PDCM-1
    cue-card: { title: "PDCM 1: 11.-03 / -998.-03" }
    sides: {}
keys:
  - key: 1 2nd Write
  - card: { mem: PDCM-1, side: 0 }
  - saveas: { card: PDCM-1, as: live, name: PDCM 1 }
```

### 5.3 Speed and waiting

| Step | Where | Meaning |
|---|---|---|
| `regular-speed:` | [both] | The items after it run at regular speed |
| `full-speed:` | [both] | The items after it run at full speed |
| `wait: <duration>` | [both] | **Wait**: let the duration pass, in emulated time, after the item before has finished; while the machine is paused, in wall-clock time. The duration is required and needs a unit: `2s`, `1.5s`, `500ms` |
| `skip-wait:` | [both] | **Skip wait**: the item before it is not waited for; the next item starts as soon as that one has been carried out. Not as the first or the last item |
| `let-it-finish:` | [both] | **Let it finish**: keep the machine running at the speed in force until it is idle, then end the preset. Only as the last item; without limit |

Their meaning is the subject of [§6](#6-speed-and-waiting).

### 5.4 Test and trace

| Step | Where | Meaning |
|---|---|---|
| `expect: <text>` | [both] | Fail the preset unless the display shows the text. Verbatim to the end of the line, like `type:`. [1600]: one line of the display contains the text; a backslash is written `\\`, a character outside ASCII `\xHH`. [59]: the text is a number, compared with the value in the display register X in its full precision (13 digits, not rounded, whatever the display format: after `π`, `3.14159265359` passes, `3.141592654` fails); blinking is not tested |
| `not-expect: <text>` | [both] | The inverse of `expect:`: fail the preset if the display shows the text, by the same comparison; passes whatever else it shows |
| `expect: /<regex>/`, `not-expect: /<regex>/` | [1600] | A text between slashes is a regular expression (ECMAScript), matched somewhere in one line of the display: `not-expect: /ERROR 1\b/` fails on ERROR 1, not on ERROR 10. A literal text beginning with `/` is written `\/`. [59]: refused |
| `screenshot: <file name>` | [both] | Save an image of the display into the product's folder for screenshots, where the GUI saves its screenshots (the save folder). No path separator |
| `trace: <file name>` / `trace: off` | [both] | Start a trace into that file in the trace folder, or stop it. No path separator. A new `trace:` closes the previous one; `trace: off` with no trace open has no effect. An open trace is closed when the preset ends |
| `elapsed-time: start` / `elapsed-time: report` | [both] | Take the emulated time as the baseline; log the emulated time since the baseline (or since the start of the preset) as `m:ss.mmm` |
| `sync-clock:` | [1600] | Set the machine's clock from the host's |

- `expect:` checks after the step before has finished: after
  `- type: RUN`, when the program has ended.
- Where the trace folder and the folder for screenshots are, and where `elapsed-time: report` is logged,
  is the product's.

### 5.5 Peripheral controls

| Step | Where | Meaning |
|---|---|---|
| `printer-trace: on` / `off` | [59] | Set the PC-100C's TRACE switch (the paper trace, not a CPU trace). Needs `printer: PC-100C`. Without the step, TRACE is off after the build |
| `cut-paper:` | [both] | Cut the paper of the printer that is plugged in, as the GUI's Cut Paper does: its picture and text go to the clipboard first, then it is cut. On the Web, and in a host without a clipboard (the harness), it is only cut. Each printer keeps its own paper; the others stay as they are. Without a printer: no effect. Never fails |
| `copy-paper:` | [both] | Copy the paper of the printer that is plugged in to the clipboard, as the GUI's Copy Paper does; the paper stays. On the Web, and in a host without a clipboard, and on an empty paper: no effect. Without a printer: no effect. Never fails |

### 5.6 Debugger

| Step | Where | Meaning |
|---|---|---|
| `pause: program` / `cpu` | [59]; [undefined] on the Sharp models | **Pause** the machine at that level. Already paused: no effect |
| `pause-on-start: program` / `cpu` / `off` | [59]; [undefined] on the Sharp models | Pause when a program starts: `program` before its first program step, `cpu` when the ROM leaves its keyboard scan loop. `off` disarms both |
| `step: { level: program\|cpu, count: n }` | [59]; [undefined] on the Sharp models | Step n times (default 1, at most 100000) while paused: a program step or an instruction. Not paused: no effect, and a line in the log |
| `resume:` | [59]; [undefined] on the Sharp models | **Resume** |

- The levels: `cpu` is one instruction, `program` one program step.
- **While paused**, a step that needs the machine to run (`key:`,
  `card:`, a `program:` section, …) waits until it runs again, by a
  `resume:` or by the user. Debugger and frontend steps go on, so `step:`
  and `resume:` work. A preset that ends paused stays paused.
- **`wait:` while paused** counts wall-clock time, not the machine's
  (§6.1), and ends with the machine still paused. `let-it-finish:` waits
  until the machine runs again and becomes idle.
- **On the Sharp models** these steps would have to name a debug target
  (the PC-1600 has a Z-80 and an LH5803), which is not defined.

### 5.7 Frontend

Steps that act on a frontend, not on the machine. A frontend that doesn't
have the panel concerned ignores the step, and so does the harness. A
frontend step never fails; it has finished when it has taken effect.

| Step | Where | Meaning |
|---|---|---|
| `debug-panel: <view>` | [59] | Show that tab of the debug panel: `calculator`, `cpu`, `log`; `calculator` is ignored while an ASM overlay runs. The Sharp models have no such step: their debug panel, the INSPECTOR, is always shown |

`debug-panel:` only chooses what is shown. `pause: cpu` pauses the
machine whichever view is shown.

## 6. Speed and waiting

### 6.1 The rules

- A machine under a preset is in one of three states: **full speed**,
  **regular speed** or **paused**. A preset runs at full speed unless it
  says otherwise; the debugger steps (§5.6) or the user pause it.
- `regular-speed:` makes the items after it run at regular speed, until
  a `full-speed:`; `full-speed:` does the opposite. They stand between
  other steps, so that the user can watch part of a preset happen in
  real time. Several in a row: the last one counts. Speed steps stand in
  `keys:` sections.
- **The implicit wait.** Between two items the preset waits until the
  item before has **finished** (§6.2), which for most items means until
  the machine is idle, at most `timeout:` (§6.3). It waits in the state
  in force.
- **`skip-wait:`** drops the implicit wait after the item before it: the
  next item starts as soon as that item has been carried out. It is for
  a step that works on a running machine, such as BREAK or R/S; most
  steps need a machine in a defined state and so the implicit wait.
- **`wait: <duration>`** lets time pass after the implicit wait (or at
  once, after a `skip-wait:`), so that a display can be seen or a
  running program goes on. The time is the machine's emulated time at
  full and at regular speed; at full speed it passes as fast as the host
  allows. While the machine is paused, it is wall-clock time.
- **The last item** is carried out (the keys pressed, ENTER typed, the
  card passed) and not waited for: the preset ends at once, and what the
  item started runs on at regular speed, under the user's control.
- **`let-it-finish:`**, allowed only as the last item, is the exception:
  the machine runs on in the state in force until it is idle, without a
  limit, and the preset ends then.
- **A preset always ends at regular speed**, whatever the speed was
  before it was loaded; a preset that ends paused stays paused (§5.6).
  Once the user has control, full speed is a gesture of the frontend
  (holding the display), not part of the preset.
- While a preset runs, the speed is the preset's; the user's gesture
  doesn't change it, and the user's key presses on the machine are
  dropped.
- **Ending a preset early.** A reset, a cold boot and a model change
  are the only actions that end a running preset; they end it at once,
  without an error, whether it hangs or not. This is the way out of a
  wait without limit (`let-it-finish:`, or the implicit wait on a paused
  machine). Everything else that would start something new, loading
  another preset included, is refused while a preset runs.

**Example.** With `- full-speed:`, `- key: A`, `- key: B`, the program
that A starts runs to its end at full speed. B is pressed after that,
and the preset ends at once: B's program runs on at regular speed, under
the user's control. With `- let-it-finish:` after `- key: B`, B's
program also runs to its end at full speed before the preset ends. With
`- skip-wait:`, `- wait: 2s` between A and B, B is pressed 2 s after A,
while A's program still runs.

### 6.2 What "finished" means

The implicit wait waits for this between two items; not after the last
item, and not after an item followed by `skip-wait:`.

| Item | Finished when |
|---|---|
| `key:` | the last press is done and the machine is idle |
| `type:` | ENTER is done and the machine is idle |
| `program:` | the program is in memory; for machine code that starts itself, when the code has returned |
| `data:`, `partition:`, `cue-card:` | the contents are written or the cue card is fitted |
| `card:` | the card has passed through the reader and is fitted |
| `arm:` | the card is fitted |
| `wait:` | the duration has passed |
| every other step | it has taken effect |

**Idle** has the meaning of the glossary: the machine waits for the user
and nothing is busy, without a break for the product's stretch; a program
that waits for a key (INPUT, a PRINT stop, INKEY$) is idle too, and so is,
on the TI-59, a machine waiting for a magnetic card (after `Write`)
(§5.2). A paused machine is not idle; the item finishes after the
machine has been resumed (§5.6).

Every product answers the same four questions, from four signals its
machine gives (key readiness, a medium awaited, what keeps it busy, the
program ended), in the same way:

| Question | Answer | Asked by |
|---|---|---|
| A, takes keys | key readiness: the ROM or a program reads the keyboard as input now; watching for BREAK alone is not A | a press that waits for it before it goes down (the product's typing profile says which); the start-up, Reset and a power cycle, which run to it |
| W, waits | A, or a medium awaited with nothing on its way; and nothing keeps the machine busy | the pacing: outside an operation, full speed runs at real time while W holds |
| settled | W without a break for the product's stretch, asked every sample | the implicit wait, `key:`, `type:`, the harness's `wait` |
| B, finished | W and the program has ended, without a break for the stretch | `let-it-finish:`, the wait "until the program has ended" |

The harness's `idle-state` shows A, W, `ended` (W and the program has
ended, now) and the busy list. Which state of the ROM gives each signal,
and the stretch and the sample, are the product's (Calc-U-59
`docs/developer/Idle.md`: 100 ms of emulated time, asked every 1 ms;
Calc-U-1600 `docs/developer/Idle.md`: 20 frames, asked every frame).

So a key that starts a program finishes when the program stops:
`full-speed:` before it runs the whole program at full speed,
`regular-speed:` before it lets the user watch the whole run.

### 6.3 Timeout

`timeout:` (header key, default `3600`) is the longest the implicit wait
lasts, in seconds of **emulated** time. An item that hasn't finished by
then fails, and so ends the preset ([README.md §4](README.md#4-errors)).

- It applies at both speeds. It doesn't apply to the last item, which is
  not waited for, nor to `let-it-finish:`, which has no limit.
- Emulated time doesn't pass while the machine is paused.
- A program that never stops (a game loop, a loop polling the keyboard),
  or one meant to be stopped with BREAK or R/S, either runs as the last
  item, or is followed by `skip-wait:`.

### 6.4 Behaviour that depends on speed

- **Adv** on the PC-100C repeats while held, counted in CPU steps, not
  in real time. At full speed one press feeds many lines; a preset puts
  `regular-speed:` before it.
- **`sync-clock:`** belongs last: the clock runs ahead while the preset
  runs at full speed.
- **`elapsed-time:`** measures emulated time; the speed doesn't change
  it.

## 7. Which keyword applies where

| Keyword | TI-59 | TI-58 | TI-58C | PC-1500 | PC-1500A | PC-1600 |
|---|---|---|---|---|---|---|
| `format-version`, `model`, `preset-name`, `description`, `timeout` | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `printer` | PC-100C | PC-100C | PC-100C | CE-150 | CE-150 | CE-150, CE-1600P |
| `slot-1` | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `slot-2`, `host-drive`, step `floppy` | | | | | | ✓ |
| `slot-1-rom` | | | | ✓ | ✓ | ✓ |
| `slot-2-rom` | | | | | | ✓ |
| `interface`, `bus-rom`, `debug` | | | | ✓ | ✓ | ✓ |
| `magnetic-cards` | ✓ | | | | | |
| steps `cassette`, `eject` | | | | ✓ | ✓ | ✓ |
| `program` | keycodes | keycodes | keycodes | BASIC, code | BASIC, code | BASIC, code |
| `data` | ✓ | ✓ | ✓, with H00–H03 | undefined | undefined | undefined |
| `partition`, `cue-card` | ✓ | ✓ | ✓ | | | |
| `keys`, `key`, speed steps, `wait`, `skip-wait`, `let-it-finish`, `expect`, `screenshot`, `trace`, `elapsed-time`, `cut-paper`, `copy-paper`, `not-expect` | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `type`, `sync-clock` | | | | ✓ | ✓ | ✓ |
| `card`, `arm` | ✓ | | | | | |
| `saveas` | `card` | | | `slot-1`, `cassette` | `slot-1`, `cassette` | `slot-1`, `slot-2`, `floppy`, `cassette` |
| `printer-trace`, `debug-panel` | ✓ | ✓ | ✓ | | | |
| `pause`, `pause-on-start`, `step`, `resume` | ✓ | ✓ | ✓ | undefined | undefined | undefined |

## 8. Key IDs

Key IDs are compared in any case. They are separated by spaces in a
`key:` step, so no key ID contains a space.

### 8.1 TI-58/58C/59

A key's ID is the label on the key when that label is one plain word;
otherwise it is the name the PC-100C prints for it. Every name the
PC-100C prints for a keycode is accepted (§8.1.1), and so is an ASCII
spelling for each name with characters outside ASCII.

**Keys**, with their position (row × 10 + column):

| Position | ID | Also | | Position | ID | Also |
|---|---|---|---|---|---|---|
| 11–15 | `A` `B` `C` `D` `E` | | | 61 | `GTO` | |
| 21 | `2nd` | `2ND` | | 62–64 | `7` `8` `9` | |
| 22 | `INV` | | | 65 | `×` | `x`, `*` |
| 23 | `lnx` | `LNX` | | 71 | `SBR` | |
| 24 | `CE` | | | 72–74 | `4` `5` `6` | |
| 25 | `CLR` | | | 75 | `-` | |
| 31 | `LRN` | | | 81 | `RST` | |
| 32 | `x⇄t` | `X⇄T`, `x<>t` | | 82–84 | `1` `2` `3` | |
| 33 | `x²` | `X²`, `x^2` | | 85 | `+` | |
| 34 | `√x` | `√X`, `sqrt` | | 91 | `R/S` | |
| 35 | `1/x` | `1/X` | | 92 | `0` | |
| 41–44 | `SST` `STO` `RCL` `SUM` | | | 93 | `.` | |
| 45 | `yˣ` | `Yˣ`, `y^x` | | 94 | `+/-` | |
| 51–54 | `BST` `EE` `(` `)` | | | 95 | `=` | |
| 55 | `÷` | `/` | | | | |

**Second functions.** The ID presses 2nd and then the key. `2nd <ID>`
means the same, so `Pgm`, `2nd Pgm` and `2nd LRN` all press 2nd, then
LRN.

| Key | ID | Also | | Key | ID | Also |
|---|---|---|---|---|---|---|
| A–E | `A'` `B'` `C'` `D'` `E'` | `A′` … `E′` | | GTO | `Pause` | `PAU` |
| lnx | `log` | `LOG` | | 7 | `x=t` | `EQ` |
| CE | `CP` | | | 8 | `Nop` | `NOP` |
| CLR | — (write `2nd CLR`) | | | 9 | `Op` | `OP` |
| LRN | `Pgm` | `PGM` | | × | `Deg` | `DEG` |
| x⇄t | `P/R` | `P→R` | | SBR | `Lbl` | `LBL` |
| x² | `sin` | `SIN` | | 4 | `x≥t` | `GE`, `x>=t` |
| √x | `cos` | `COS` | | 5 | `Σ+` | `∑+`, `sum+` |
| 1/x | `tan` | `TAN` | | 6 | `x̄` | `xbar` |
| SST | `Ins` | `INS` | | - | `Rad` | `RAD` |
| STO | `CMs` | `CMS` | | RST | `STF` | |
| RCL | `Exc` | `EXC` | | 1 | `IFF` | |
| SUM | `Prd` | `PRD` | | 2 | `D.MS` | `DMS` |
| yˣ | `Ind` | `IND` | | 3 | `π` | `pi` |
| BST | `Del` | `DEL` | | + | `Grad` | `GRD` |
| EE | `Eng` | `ENG` | | R/S | `Write` | `WRT` |
| ( | `Fix` | `FIX` | | 0 | `Dsz` | `DSZ` |
| ) | `Int` | `INT` | | . | `Adv` | `ADV` |
| ÷ | `\|x\|` | `IxI` | | +/- | `Prt` | `PRT` |
| | | | | = | `List` | `LST` |

- `St flg` and `If flg` contain a blank, so their IDs are the printer's
  names, `STF` and `IFF`.
- `CLR`'s second function has the same printer name as `CLR`, so the bare
  ID `CLR` is the key, and its second function is written `2nd CLR`.
- Merged program codes are key sequences; their printer names press
  them (§8.1.1).

#### 8.1.1 Printer names

Every name the PC-100C prints for a keycode is accepted in a `key:` step,
in any case. Most are the IDs or "Also" names above. The others:

| Keycode | Printer name | Presses |
|---|---|---|
| 20 | `CLR` | `CLR` (the key; its second function is `2nd CLR`) |
| 26, 27 | `2ND`, `INV` | `2nd`, `INV` |
| 62 | `PG*` | `Pgm Ind` |
| 63 | `EX*` | `Exc Ind` |
| 64 | `PD*` | `Prd Ind` |
| 72 | `ST*` | `STO Ind` |
| 73 | `RC*` | `RCL Ind` |
| 74 | `SM*` | `SUM Ind` |
| 83 | `GO*` | `GTO Ind` |
| 84 | `OP*` | `Op Ind` |
| 92 | `RTN` | `INV SBR` |
| 82 | `HIR` | — (no key sequence enters it; an error) |

ASCII spellings of the printer names with characters outside ASCII:
`X⇄T` `x<>t`, `X²` `x^2`, `√X` `sqrt`, `Yˣ` `y^x`, `÷` `/`, `∑+` `sum+`,
`x̄` `xbar`, `π` `pi`.

### 8.2 PC-1500/1500A/1600

- a single letter, digit, or `+ - * / = ( ) .`;
- `enter`, `space`, `cl`, `mode`, `shift`, `def`, `sml`, `rcl`, `rsv`,
  `off`, `up`, `down`, `left`, `right`, `f1`–`f6`, and `on` (or `break`).
- on the PC-1600 also `ctrl`, `kbii` and `bs`.

## 9. From version 1

For converting files written for the products before this specification.

### 9.1 Calc-U-59 state file

| Version 1 | Version 2 |
|---|---|
| `MODEL: TI-59` (optional) | `model: TI-59` (required) |
| `SOLID-STATE-MODULE: ML` (absent: unchanged) | `slot-1: { name: ML }` (absent: none) |
| `PRINTER: on` (absent: unchanged) | `printer: PC-100C` (absent: none) |
| `PARTITION:`, `PROGRAM:`, `CUECARD:` | `partition:`, `program: { keycodes: … }`, `cue-card:` |
| `REGISTERS:` lines `NN = value` | `data:` entries `NN: value` |
| sections after `KEYSTROKES:` (reload) | sections in file order |
| `CARD: mem:X` … `END-CARD` | an item of `magnetic-cards:` |
| `KEYSTROKES:` with matrix codes `21 31` | `keys:` with `- key: Pgm` (positions: §8.1) |
| `Wait: 2s` | `- wait: 2s` |
| `FullSpeed:` / `RegularSpeed:` (default regular) | `- full-speed:` / `- regular-speed:` (default full); `- regular-speed:` first keeps the version 1 speed |
| the last key is waited for until the machine is idle, then regular speed | the last item is not waited for; `- let-it-finish:` last keeps a run at full speed to its end |
| `Trace: x` / `Trace: Off` | `- trace: x` / `- trace: off` |
| `ZeroElapseTime:` / `ReportElapseTime:` | `- elapsed-time: start` / `report` |
| `InsertCard: X:2` | `- card: { file: X.magnetcard.yaml, side: 1 }` (sides are numbered 0 and 1; version 1's 1 and 2 are 0 and 1) |
| `InsertCard:` / `ArmCard:` on a card file that doesn't exist yet (created when written, with the latest `CUECARD:`) | an in-file card with that `cue-card`, fed with `mem:`, then `- saveas: { card: X, as: live, name: X }` |
| `ArmCard: mem:X` (fed when the program waits) | `- card: { mem: X }` after the step that makes the machine wait for a card; or `- arm: { mem: X }` for the user to feed |
| a card fed by `InsertCard:` / `ArmCard:`: a `mem:` card is not fitted | every card is fitted after it has passed |
| `PrinterTrace: on` | `- printer-trace: on` |
| `CutPaper:`, `DebugPanel: CPU` | `- cut-paper:`, `- debug-panel: cpu` |
| `Freeze: Calculator`, `FreezeOnStart: CPU`, `Step: CPU 5`, `Resume:` | `- pause: program`, `- pause-on-start: cpu`, `- step: { level: cpu, count: 5 }`, `- resume:` |
| a bad line is skipped | the file is rejected |
| registers 60–99 on a TI-58/58C are ignored | an error |

### 9.2 Calc-U-1600 preset version 1

| Version 1 | Version 2 |
|---|---|
| `format-version: 1` | `format-version: 2` |
| `plotter: CE-1600P:old` | `printer: CE-1600P:old` |
| `slot-1: CE-1600M` / `slot-1-file: my.card.yaml` | `slot-1: { name: CE-1600M }` / `slot-1: { file: my.card.yaml }` |
| `slot-1-rom: build/x.bin` | `slot-1-rom: { file: build/x.bin }` |
| `floppy: Formatted,B` / `floppy-file: d.floppy.yaml` (header) | the step `- floppy: { name: Formatted, side: 1 }` / `- floppy: { file: d.floppy.yaml }` in a `keys:` section that comes first (A and B are sides 0 and 1) |
| `host-drive: shared` | `host-drive: { folder: shared }` |
| `- key: cl` (one key) | `- key: cl`, or several: `- key: shift up up` |
| `- wait: 2` (seconds) | `- wait: 2s` |
| `- wait:` (half a second, then until idle, at most 1 h) | last: `- let-it-finish:` (without limit); elsewhere dropped, since the implicit wait does it |
| `- wait: 2` to let a running program run before the next step (BREAK) | `- skip-wait:`, `- wait: 2s` |
| `- type:` waits after ENTER only until BUSY clears, at most 1 s (PC-1600) or the machine settles, at most 12 s (PC-1500); `- key:` doesn't wait | every item but the last is waited for until the machine is idle, within `timeout:` |
| the last `- type:` at full speed is waited for as above | not waited for; `- let-it-finish:` last keeps it |
| `- saveas: live protected slot-1:My ROM` | `- saveas: { place: slot-1, as: live, protected: true, name: My ROM }` |
| `- saveas: template slot-2:file:C.card.yaml` | `- saveas: { place: slot-2, as: template, file: C.card.yaml }` |
| `- expect:` right after `- type: RUN` checks at once | checks when the program has ended |
| full speed throughout; afterwards the user's earlier speed | full speed unless `regular-speed:`; afterwards regular speed |
| `- syncclock:` | `- sync-clock:` |

### 9.3 Calc-U-59 card files, cue cards, constant memory, modules

| Version 1 | Version 2 |
|---|---|
| card file `Calc-U-59 Card 2.0` (`.U59`), one per card, ending with `END-CARD` | magnetic card format 3 ([media.md §1](media.md#1-magnetic-card)), `.magnetcard.yaml`; `card-name` from the file stem; no end line |
| the card's `CUECARD:` block | `cue-card:` (its keys below) |
| `SIDE: n` (1 or 2) | `sides:` `n−1:` (0 or 1) |
| `HEADER:` keys `Partition`, `DataType`, `Bank`, `Protection`, `Checksum` | the side's keys `partition`, `data-type`, `bank`, `protection`, `checksum`, every one required |
| `Partition` and `Checksum`: a byte in hex without a prefix (`18`) | `0x18`: the prefix is needed, since a number without it is read as decimal |
| `DataType` `program` / `data`, `Bank` 1–4 | the same words and numbers |
| `Protection` `yes` / `no` | `true` / `false` |
| `DATA:` rows `R###: …` | `encoding: register-hex`, `bytes:` the 30 rows copied unchanged, in the same order (labels and bytes) |
| card files 1.0 and the binary layouts | not converted |
| `ti58c.mem` (cue card, `R###:` rows) | constant memory format 1 ([media.md §4](media.md#4-constant-memory-file)), rows copied unchanged |
| `roms/solid-state/cuecards.txt` + one `TMC05xx.txt` per module | one Solid State ROM module file per module ([modules.md §2](modules.md#2-solid-state-rom-module)): `MODULE-ID` → `module-name`, `MODULE-MENU-TITLE` → `description`, `MODULE-TITLE` → `title`, `MODULE-MENU-SORT` → `menu-sort`, `CUECARD: n` → a `cue-cards:` item with `program: n`. The dump's decimal row addresses become hex `addressed-hex` addresses; its header lines go into `notes` |
| `CUECARD:` keys `Title`, `ID`, `Style`, `Row1`, `Row2`, `Row2R`, `Row1Align`, `Row2Align`, `Row2RAlign`, `PencilColor` | `title`, `id`, `style`, `row-1`, `row-2`, `row-2-r`, `row-1-align`, `row-2-align`, `row-2-r-align`, `pencil-color` ([cue-card.md](cue-card.md)) |
| `CUECARD:` keys `Template` and `Banks` | dropped: the kind follows from where the card is defined; a magnetic card's corners show its sides' banks |
| `CUECARD:` unknown keys and `IDAlign` are ignored | an error |

A card the ROM wrote: `PDCM-1.U59`, written by Calc-U-59 2.3.0's
example `historic/pdcm-make-cards.ti59` (side 1; rows R002 to R029 left
out here):

```text
Calc-U-59 Card 2.0

CUECARD:
Template: MagnetCard
Title: PDCM 1: 11.-03 / -998.-03

SIDE: 1
HEADER:
Partition: 18
DataType: program
Bank: 1
Protection: no
Checksum: 03

DATA:
R000: 30 19 75 49 90 90 90 39
R001: 23 49 80 90 90 39 69 22
```

The same card as Calc-U-59 writes it in format 3, `PDCM-1.magnetcard.yaml`,
from today's example of that name (`saved` is the time of the write; the
same rows left out, side 1 not shown):

```yaml
# Calc-U-59 magnetic card
format: ti59-magnetic-card
format-version: 3
card-name: "PDCM-1"
saved: 1999-12-31T23:00:00Z
cue-card:
  title: "PDCM 1: 11.-03 / -998.-03"
sides:
  0:
    partition: 0x18
    data-type: program
    bank: 1
    protection: false
    checksum: 0x03
    encoding: register-hex
    bytes: |
      R000: 30 19 75 49 90 90 90 39
      R001: 23 49 80 90 90 39 69 22
```

### 9.4 Calc-U-1600 floppy disk version 1

| Version 1 | Version 2 |
|---|---|
| `format-version: 1` | `format-version: 2` |
| `sides:` with `a:` and `b:` | `sides:` with `0:` and `1:`, the same bytes |

## 10. Examples

A TI-59 preset that reads two cards, then runs a program the user
watches:

```yaml
format-version: 2
model: TI-59
preset-name: Card demo
description: Reads two banks, then runs at regular speed
printer: PC-100C
magnetic-cards:
  - card-name: Bank1
    sides:
      0:
        bank: 1
        program: |
          76 11 99 91
keys:
  - card: { mem: Bank1 }
  - card: { file: cards/bank2.magnetcard.yaml }
  - regular-speed:
  - key: A                     # the last item, at regular speed: the preset ends here
```

A PC-1600 preset:

```yaml
format-version: 2
model: PC-1600
preset-name: Hanoi
description: Towers of Hanoi, up to seven bricks
printer: CE-1600P
slot-1: { name: CE-1600M }
keys:
  - floppy: { name: Formatted }
  - key: mode
  - type: NEW0
program:
  file: hanoi.bas
keys:
  - key: mode
  - sync-clock:
  - type: RUN                  # the last item: the game runs at regular speed
```

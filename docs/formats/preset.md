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
   A hardware key that is absent means **none**, on every model.
2. **Cold boot** it: all memory empty, the constant memory of a TI-58C
   included.
3. **Carry out the sections** in file order (§4). Each item starts only
   after the item before it has finished ([§6](#6-full-speed)).
4. **End.** The machine runs on at regular speed, under the user's
   control.

Without a `cue-card:` section, no cue card is fitted.

## 3. Header keys

| Key | Where | Value |
|---|---|---|
| `format-version` | [both] | `2`. Required |
| `model` | [both] | Required. `TI-59`, `TI-58`, `TI-58C` [59]; `PC-1500[:A01\|A03\|A04]` (default `A04`), `PC-1500A` (ROM A04 only), `PC-1600[:new\|old]` (default `new`) [1600]. Any case. The part after `:` is the **variant** |
| `preset-name` | [both] | The name tag. Optional; default: the file stem |
| `description` | [both] | One line for lists, menus and tooltips |
| `timeout` | [both] | The longest an item may take to finish, in seconds of emulated time ([§6.3](#63-timeout)). Optional; default `3600` |
| `printer` | [both] | `PC-100C` [59]; `CE-150` [1600]; `CE-1600P[:new\|old]` (default `new`) [1600: PC-1600]. Any case |
| `interface` | [1600] | `CE-158` |
| `slot-1` | [both] | A module, as a reference by `file:` or `name:`. [59]: a Solid State ROM module ([modules.md §2](modules.md#2-solid-state-rom-module)); `name:` is its catalogue ID. [1600]: a module definition ([modules.md §1](modules.md#1-module-definition)); `name:` is its `module-name` |
| `slot-2` | [1600: PC-1600] | As `slot-1` |
| `slot-1-rom`, `slot-2-rom` | [1600] | A ROM file, by `file:` only, whose bytes replace the ROM of the module in that slot (§3.2) |
| `floppy` | [1600: PC-1600 with `printer: CE-1600P`] | A floppy disk ([media.md §2](media.md#2-floppy-disk)), by `file:` or `name:`, with `side: A\|B` (default `A`). The CE-1600F drive comes with the CE-1600P |
| `host-drive` | [1600: PC-1600] | The folder that backs drive `S3:`, by `folder:` |
| `tape` | [undefined] | A cassette tape ([media.md §3](media.md#3-cassette-tape)); undefined until the tape position and the recorder are specified (OQ-4) |
| `bus-rom` | [1600] | ROMs on the 60-pin connector (§3.3) |
| `debug` | [1600] | The debugger configuration of a project preset (§3.4) |
| `magnetic-cards` | [59: TI-59] | Magnetic cards defined inside the preset (§3.1) |

```yaml
format-version: 2
model: PC-1600:new
printer: CE-1600P
floppy: { name: Formatted, side: A }
slot-1: { name: CE-1600M }
slot-2: { file: cards/my.card.yaml }
```

### 3.1 `magnetic-cards` [59: TI-59]

A list of **in-file cards**: magnetic cards defined by the preset, which
a `card:` or `arm:` step addresses with `mem: <card-name>`. An in-file
card is never saved to a file. A bank written to it is kept, and read
back by its next use, until the next load, reset or model switch.

```yaml
magnetic-cards:
  - card-name: MyVars
    cue-card:                 # optional
      title: My Vars
    sides:
      1:
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
  `sides` with `1`, `2` or both. A side not given is fresh (unwritten).
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
  **fresh** in-file card.
- Two items with one `card-name` are an error.

### 3.2 `slot-N-rom` [1600]

While a ROM module is being developed, its bytes come from the build:

```yaml
slot-1: { file: module.card.yaml }     # decode, banking, the ROM's size
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
ignores the block. Its keys:

| Key | Meaning |
|---|---|
| `preset` | A preset for the clean start; default: the model's startup preset |
| `program` | The program to load: `bin`, `listing`, `source` (if the main source isn't `<listing>.asm`), `symbols`, `cpu` (`lh5801` / `z80` / `lh5803`), `address`, `entry`, `after` (`stopOnEntry` / `call` / `none`), `cleanStart` (default `true`), and the bank qualifiers `bank`, `me`, `pu`, `pv` |
| `listings` | Listings of code that isn't loaded (ROM): a path, or `{path, source, cpu, bank, me, pu, pv}` |
| `symbols` | Symbol tables: a path, or `{path, cpu, bank, me, pu, pv}` |
| `command` | A BASIC line typed to start the code: in place of the program's `CALL`, or, without a program, once the machine is set up |
| `boot` | `debug`: run the power-on under the debugger (the preset's hardware only; no `program` or `command`) |
| `reset` | `reset` / `allReset`: reset without the boot, and start at the reset vector |
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
- `file: <path>`: a file holding the program in the same notation.

```yaml
program:
  keycodes: |
    76 11 42 00              # bare keycodes, any number per line
    000 43 RCL               # step number, keycode, then an optional label
    001 00 00
    110 42 STO               # a step number moves the position
    ...                      # a gap: the steps it stands for are not written
```

- **Bare keycodes:** every token of 1–2 digits with a value 0–99 is a
  keycode. Other tokens (mnemonics) are labels and are skipped.
- **Step-numbered:** a first token of 3 or more digits is a step number
  (0 up to the model's last physical step: 959 on the TI-59, 479 on the
  TI-58/58C). It sets the position. The next token is the one keycode;
  anything after it is a label, even a number (`473 00 0`).
- **Gap:** a line that is exactly `...` writes nothing; the position
  moves on at the next step number.
- **Only the listed steps are written.** Every other step keeps its
  contents (00 after the cold boot).
- Each `program:` section starts at step 000.
- Keycodes are not key IDs: digit 1 is keycode `01`, and `Pgm` is the
  one step `36`. Key IDs are for `key:` steps (§8).

**[1600]: a BASIC or machine-code program.**

| Key | Value |
|---|---|
| `file: <path>` | Loaded by what the file holds: a BASIC listing (`.bas`), tokenized BASIC, or machine code with or without a header |
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
- Other file kinds: OQ-7.

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
above the top row of keys, as the machine's own card ([cue-card.md §2](cue-card.md#2-fitting)). A preset's cue card
also goes onto every magnetic card file the preset writes, with its whole
layout ([media.md §1.4](media.md#14-written-cards)).

## 5. Steps

A `keys:` section holds a list of steps, each `- verb: value` or, for a
verb without a value, `- verb:`. **Every step starts only after the step
before has finished** ([§6](#6-full-speed)).

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
| `key: <id> [<id> …]` | [both] | **Press**: press and release each key, one after the other. Each press waits for key readiness before it goes down. Key IDs (§8) are separated by spaces and compared in any case |
| `type: <text>` | [1600] | **Typing**: type the text as key presses, then ENTER. The text runs verbatim to the end of the line: `#` and quotes are typed, not read as a comment or quotes |

- `key:` presses keys; it doesn't type text. `key: PRINT` is an error,
  not five letters; text is typed with `type:`. Several keys on one line
  are still separate presses.
- A word that is not a key ID is an error. On the TI models a two-digit
  number such as `12` is an error; `1 2` is two digit keys.
- The PC-1600 keys `ctrl`, `kbii` and `bs` can't be pressed from a
  preset.

### 5.2 Media

| Step | Where | Meaning |
|---|---|---|
| `card: <reference>` | [59: TI-59] | **Insert** a magnetic card: push one side through the card reader. Reference by `file:`, `name:` or `mem:`, with `side: 1\|2` (default 1) |
| `arm: <reference>` | [59: TI-59] | **Arm** a magnetic card: make it ready for the user to feed. Reference by `file:`, `name:` or `mem:`, without a side |
| `saveas: <target>` | [1600] | **Name & Save** the medium or module at a place (§5.2.1) |

**`card:`** is the user pushing a card into the calculator. The ROM
reads the side, or writes it if the machine waits for a card to write.
The step has finished when the card has passed. **After it has passed,
the card is fitted** ([cue-card.md §2](cue-card.md#2-fitting)).

- The TI-59 takes a card when it is idle (in its keyboard loop), and when
  it explicitly waits for one (after `Write`). Both count as idle
  ([§6.2](#62-what-finished-means)), so `- key: 3 2nd Write` finishes
  when the machine waits for the card, and the next step, `card:`, feeds
  it. A card inserted while the machine can't take one is the ROM's
  matter, as on the real calculator.
- The ROM checks what it checks on real hardware: the partition on a
  read at idle, the bank number on `n INV Write`. Choosing the right
  card is the preset's job.
- A `file:` reference to a file that doesn't exist is a fresh card. The
  file is created when a side is written, with its file stem as
  `card-name`.

**`arm:`** makes the card ready for the user to feed by hand: the card
is fitted, and the user feeds it by tapping a corner. Arming does nothing
electrically (no card is detected by the machine) and doesn't read the
card, so no side is chosen. A preset inserts cards with `card:`; it arms
one only to leave it to the user, typically as its last step.

#### 5.2.1 `saveas` [1600]

```yaml
- saveas: { place: slot-1, as: live, name: My card }
- saveas: { place: slot-1, as: live, protected: true, name: My ROM }
- saveas: { place: floppy, as: template, file: disks/Work.floppy.yaml }
```

| Key | Value |
|---|---|
| `place` | `slot-1`, `slot-2` [PC-1600] or `floppy` [PC-1600]: the place whose medium or module is saved |
| `as` | `live`: an instance, saved again after each change. `template`: a template, never written again |
| `protected` | `true`: save with the module's write-protect switch on, and switch it on in the machine. Only for a module that has the switch (CE-161); not for a floppy disk |
| `name` / `file` | Exactly one. `name`: the name tag of the saved copy, which goes into the save folder. `file`: the file to write; its name tag is the file name without the suffix (`.card.yaml`, `.floppy.yaml`), which it must have |

- A name must not contain `"`. A saved copy never takes the name of a
  bundled template.
- A module with a `slot-N-rom:` can't be saved.
- The TI-59 has nothing to save: its card reader is pass-through, and the
  program writes magnetic cards itself.

### 5.3 Speed

| Step | Where | Meaning |
|---|---|---|
| `regular-speed:` | [both] | The items after it run at regular speed |
| `full-speed:` | [both] | The items after it run at full speed |

Their meaning is the subject of [§6](#6-full-speed).

### 5.4 Test and trace

| Step | Where | Meaning |
|---|---|---|
| `expect: <text>` | [both] | Fail the preset unless the display shows the text. Verbatim to the end of the line, like `type:`. [1600]: one line of the display contains the text; a backslash is written `\\`, a character outside ASCII `\xHH`. [59]: the display read as text (OQ-5) |
| `screenshot: <file name>` | [both] | Save an image of the display into the trace folder. No path separator |
| `trace: <file name>` / `trace: off` | [both] | Start a trace into that file in the trace folder, or stop it. No path separator. A new `trace:` closes the previous one. An open trace is closed when the preset ends |
| `elapsed-time: start` / `elapsed-time: report` | [both] | Take the emulated time as the baseline; log the emulated time since the baseline (or since the start of the preset) as `m:ss.mmm` |
| `syncclock:` | [1600] | Set the machine's clock from the host's |

- `expect:` checks after the step before has finished: after
  `- type: RUN`, when the program has ended.
- Where the trace folder is, and where `elapsed-time: report` is logged,
  is the product's.

### 5.5 Peripheral controls

| Step | Where | Meaning |
|---|---|---|
| `printer-trace: on` / `off` | [59] | Set the PC-100C's TRACE switch (the paper trace, not a CPU trace). Needs `printer: PC-100C` |

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
- **On the Sharp models** these steps would have to name a debug target
  (the PC-1600 has a Z-80 and an LH5803), which is not defined.

### 5.7 Frontend

Steps that act on a frontend, not on the machine. A frontend that doesn't
have the panel concerned ignores the step, and so does the harness. A
frontend step never fails; it has finished when it has taken effect.

| Step | Where | Meaning |
|---|---|---|
| `debug-panel: <view>` | [both] | Show that view of the debug panel. [59]: `calculator`, `cpu`, `log`; `calculator` is ignored while an ASM overlay runs. [1600]: a view of the INSPECTOR: `pointers`, `inventory`, `z80-view` (PC-1600), `lh5803-view` (PC-1600), `lh5801-view` (PC-1500), `basic-area`, `program-areas` (PC-1600), `disk-areas` (PC-1600), `physical-ram`, `dump` (OQ-6) |
| `cut-paper:` | [both] | Clear the paper in the paper panel. Without a printer: no effect |

`debug-panel:` only chooses what is shown. `pause: cpu` pauses the
machine whichever view is shown.

## 6. Full speed

### 6.1 The rules

- A preset runs at **full speed** unless it says otherwise.
- Every **item** (a step, or a section) starts only after the item
  before it has **finished** (§6.2). There is no step to wait.
- `regular-speed:` makes the items after it run at **regular speed**,
  until a `full-speed:`; `full-speed:` does the opposite. They stand
  between other steps, so that the user can watch part of a preset happen
  in real time. At regular speed, too, each item waits for the one
  before it.
- **The last item** of a preset:
  - at full speed, runs to its end at full speed; then the preset ends
    and the user has control;
  - at regular speed, starts, and the preset ends right away: the user
    has control while the item runs on at regular speed.
- **A preset always ends at regular speed**, whatever the speed was
  before it was loaded. Once the user has control, full speed is a
  gesture of the frontend (holding the display), not part of the preset.
- While a preset runs, the speed is the preset's; the user's gesture
  doesn't change it.
- Speed steps stand in `keys:` sections. Several in a row: the last one
  counts.

**Example.** With `- regular-speed:`, `- key: A`, `- full-speed:`,
`- key: B`, the program that A starts runs to its end at regular speed.
B is pressed after that, and its program runs at full speed, to its end,
before the preset ends. Had the preset ended after `- key: A`, it would
have ended right after A was pressed, with A's program running on at
regular speed.

### 6.2 What "finished" means

| Item | Finished when |
|---|---|
| `key:` | the last press is done and the machine is idle |
| `type:` | ENTER is done and the machine is idle |
| `program:` | the program is in memory; for machine code that starts itself, when the code has returned |
| `data:`, `partition:`, `cue-card:` | the contents are written or the cue card is fitted |
| `card:` | the card has passed through the reader and is fitted |
| `arm:` | the card is fitted |
| every other step | it has taken effect |

**Idle**, per model family:

- **TI-58/58C/59:** the ROM is in its keyboard loop (key readiness). On
  the TI-59, a machine waiting for a magnetic card (after `Write`) is
  idle too.
- **PC-1500/1500A/1600:** the ROM's keyboard idle loop has taken over.

A paused machine is not idle; the item finishes after the machine has
been resumed (§5.6).

So a key that starts a program finishes when the program stops:
`full-speed:` before it runs the whole program at full speed,
`regular-speed:` before it lets the user watch the whole run.

### 6.3 Timeout

`timeout:` (header key, default `3600`) is the longest an item may take
to finish, in seconds of **emulated** time. An item that hasn't finished
by then fails, and so ends the preset ([README.md §4](README.md#4-errors)).

- It applies at both speeds, to every item the preset waits for. The
  last item at regular speed is not waited for, so no timeout applies to
  it.
- Emulated time doesn't pass while the machine is paused.
- A program that never stops (a game loop, a loop polling the keyboard),
  or one meant to be stopped with BREAK or R/S, reaches the timeout unless
  it runs as the last item at regular speed.

### 6.4 Behaviour that depends on speed

- **Adv** on the PC-100C repeats while held, counted in CPU steps, not
  in real time. At full speed one press feeds many lines; a preset puts
  `regular-speed:` before it.
- **`syncclock:`** belongs last: the clock runs ahead while the preset
  runs at full speed.
- **`elapsed-time:`** measures emulated time; the speed doesn't change
  it.

## 7. Which keyword applies where

| Keyword | TI-59 | TI-58 | TI-58C | PC-1500 | PC-1500A | PC-1600 |
|---|---|---|---|---|---|---|
| `format-version`, `model`, `preset-name`, `description`, `timeout` | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `printer` | PC-100C | PC-100C | PC-100C | CE-150 | CE-150 | CE-150, CE-1600P |
| `slot-1` | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `slot-2`, `floppy`, `host-drive` | | | | | | ✓ |
| `slot-1-rom` | | | | ✓ | ✓ | ✓ |
| `slot-2-rom` | | | | | | ✓ |
| `interface`, `bus-rom`, `debug` | | | | ✓ | ✓ | ✓ |
| `magnetic-cards` | ✓ | | | | | |
| `tape` | | | | undefined | undefined | undefined |
| `program` | keycodes | keycodes | keycodes | BASIC, code | BASIC, code | BASIC, code |
| `data` | ✓ | ✓ | ✓, with H00–H03 | undefined | undefined | undefined |
| `partition`, `cue-card` | ✓ | ✓ | ✓ | | | |
| `keys`, `key`, speed steps, `expect`, `screenshot`, `trace`, `elapsed-time`, `debug-panel`, `cut-paper` | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `type`, `syncclock` | | | | ✓ | ✓ | ✓ |
| `card`, `arm` | ✓ | | | | | |
| `saveas` | | | | `slot-1` | `slot-1` | `slot-1`, `slot-2`, `floppy` |
| `printer-trace` | ✓ | ✓ | ✓ | | | |
| `pause`, `pause-on-start`, `step`, `resume` | ✓ | ✓ | ✓ | undefined | undefined | undefined |

## 8. Key IDs

Key IDs are compared in any case. They are separated by spaces in a
`key:` step, so no key ID contains a space.

### 8.1 TI-58/58C/59

A key's ID is the label on the key when that label is one plain word;
otherwise it is the name the PC-100C prints for it. The printer's name is
always accepted as well. The ASCII spellings for labels with characters
outside ASCII are proposed (OQ-1).

**Keys**, with their position (row × 10 + column):

| Position | ID | Also | | Position | ID | Also |
|---|---|---|---|---|---|---|
| 11–15 | `A` `B` `C` `D` `E` | | | 61 | `GTO` | |
| 21 | `2nd` | `2ND` | | 62–64 | `7` `8` `9` | |
| 22 | `INV` | | | 65 | `×` | `*` |
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
- Merged program codes (`Pgm Ind`, `STO Ind`, `RTN`, …) are key
  sequences: `- key: STO Ind 0 5`, `- key: INV SBR`.

### 8.2 PC-1500/1500A/1600

- a single letter, digit, or `+ - * / = ( ) .`;
- `enter`, `space`, `cl`, `mode`, `shift`, `def`, `sml`, `rcl`, `rsv`,
  `off`, `up`, `down`, `left`, `right`, `f1`–`f6`, and `on` (or `break`).
- `ctrl`, `kbii` and `bs` (PC-1600) can't be pressed from a preset.

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
| `Wait: 2s` | — (removed) |
| `FullSpeed:` / `RegularSpeed:` (default regular) | `- full-speed:` / `- regular-speed:` (default full) |
| `Trace: x` / `Trace: Off` | `- trace: x` / `- trace: off` |
| `ZeroElapseTime:` / `ReportElapseTime:` | `- elapsed-time: start` / `report` |
| `InsertCard: X:2` | `- card: { file: X.magnetic-card.yaml, side: 2 }` |
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
| `floppy: Formatted,B` / `floppy-file: d.floppy.yaml` | `floppy: { name: Formatted, side: B }` / `floppy: { file: d.floppy.yaml }` |
| `host-drive: shared` | `host-drive: { folder: shared }` |
| `- key: cl` (one key) | `- key: cl`, or several: `- key: shift up up` |
| `- wait: 2`, `- wait:` (until idle, at most 1 h) | — (removed; every item waits until it has finished, within `timeout:`) |
| `- saveas: live protected slot-1:My ROM` | `- saveas: { place: slot-1, as: live, protected: true, name: My ROM }` |
| `- saveas: template slot-2:file:C.card.yaml` | `- saveas: { place: slot-2, as: template, file: C.card.yaml }` |
| `- expect:` right after `- type: RUN` checks at once | checks when the program has ended |
| full speed throughout; afterwards the user's earlier speed | full speed unless `regular-speed:`; afterwards regular speed |

### 9.3 Calc-U-59 card files, cue cards, constant memory, modules

| Version 1 | Version 2 |
|---|---|
| card file `Calc-U-59 Card 2.0`, one per card | magnetic card format 3 ([media.md §1](media.md#1-magnetic-card)); `card-name` from the file stem |
| `SIDE: n`, `HEADER:` keys `Partition`, `DataType`, `Bank`, `Protection`, `Checksum` | `sides: n:` keys `partition`, `data-type`, `bank`, `protection` (`yes` → `true`), `checksum` |
| `DATA:` rows `R###: …` | `encoding: register-hex`, `bytes:` rows copied unchanged |
| card files 1.0 and the binary layouts | not converted |
| `ti58c.mem` (cue card, `R###:` rows) | constant memory format 1 ([media.md §4](media.md#4-constant-memory-file)), rows copied unchanged |
| `roms/solid-state/cuecards.txt` + one `TMC05xx.txt` per module | one Solid State ROM module file per module ([modules.md §2](modules.md#2-solid-state-rom-module)): `MODULE-ID` → `module-name`, `MODULE-MENU-TITLE` → `description`, `MODULE-TITLE` → `title`, `MODULE-MENU-SORT` → `menu-sort`, `CUECARD: n` → a `cue-cards:` item with `program: n`. The dump's decimal row addresses become hex `addressed-hex` addresses; its header lines go into `notes` |
| `CUECARD:` keys `Title`, `ID`, `Style`, `Row1`, `Row2`, `Row2R`, `Row1Align`, `Row2Align`, `Row2RAlign`, `PencilColor` | `title`, `id`, `style`, `row-1`, `row-2`, `row-2-r`, `row-1-align`, `row-2-align`, `row-2-r-align`, `pencil-color` ([cue-card.md](cue-card.md)) |
| `CUECARD:` keys `Template` and `Banks` | dropped: the kind follows from where the card is defined; a magnetic card's corners show its sides' banks |
| `CUECARD:` unknown keys and `IDAlign` are ignored | an error |

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
      1:
        bank: 1
        program: |
          76 11 99 91
keys:
  - card: { mem: Bank1 }
  - card: { file: cards/bank2.magnetic-card.yaml }
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
floppy: { name: Formatted }
slot-1: { name: CE-1600M }
keys:
  - key: mode
  - type: NEW0
program:
  file: hanoi.bas
keys:
  - key: mode
  - type: RUN                  # at full speed; finished when the program waits for input
  - syncclock:
```

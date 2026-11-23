# The harness protocol

The common protocol of the harness: the program `calcu`, its one-shot
form, `calcu serve`, and harness attach to a running app. It says what a
line holds, what a reply looks like and what each common command takes
and gives. The product commands (`c59 …`, `c1600 …`) are not here: they
are in `products/calc-u-59/docs/developer/Harness.md` and
`products/calc-u-1600/docs/developer/Harness.md`.

Terms follow `docs/GLOSSARY.md`. The code is in `library/harness/`:
`Protocol.hpp` (lines), `Reply.hpp` (replies), `Commands.hpp` (the
registry), `cli/Cli.cpp` (options), `attach/` (harness attach).

## 1. Forms

```
calcu --product <name> [options] <command> [then <command> …]
calcu --product <name> [options] serve [--dap <port>]
calcu attach [--port N] [--json] [--messages]
```

| Form | Input | Output | Machine |
|---|---|---|---|
| One-shot | the commands on the command line, parted by the word `then` | one reply per command on standard output | built for the run |
| `serve` | one command per line on standard input | one reply per command line on standard output | built for the run |
| Attach | one command per line over TCP on 127.0.0.1 | replies and message lines over the same connection | the running app's |

- The run builds the machine the product starts with, under the run's
  settings, and runs it to key readiness before the first command.
- In the one-shot form the words of each command are joined with one
  space into a line; from there the one-shot form and `serve` share one
  parser. A literal argument `then` cannot be written in the one-shot
  form; `serve` takes it.
- In the one-shot form the commands are carried out in order until the
  first refused one; the commands after it are not carried out.
- `quit` ends `serve` and the one-shot form; it gets no reply. The end of
  standard input ends `serve` as `quit` does.
- The harness has no wall clock: in the one-shot form and `serve`
  emulated time passes only by commands, unpaced, so two runs with the
  same product, options and commands give the same replies byte for
  byte.
- `serve` writes nothing but replies. Notices and log lines are read with
  `state notices` and `state log`.

## 2. Lines

| Rule | |
|---|---|
| End | A line ends at `\n`; a `\r` before it is dropped |
| Space | Spaces and tabs at the start and the end are dropped |
| Empty line | No command; no reply |
| Command | The first word is a common command, or a product word (`c59`, `c1600`) followed by the product command's name |
| Argument text | The rest of the line after the command's name, as written, trimmed |
| Unknown word | `refused: unknown command <word>` |
| Product word alone | `refused: unknown command <word> (a product command needs its name)` |
| Unknown product command | `refused: unknown command <word> <name>` |

**The value of a step.** A command that is a preset step takes the
step's value exactly, by the syntax of [README.md §1](README.md#1-syntax-simplified-yaml):
a scalar, a flow mapping or a flow sequence. A step becomes the command
by dropping `- ` and the colon: `- floppy: { name: Disk2, side: 1 }` is
the line `floppy { name: Disk2, side: 1 }`. In the one-shot form the
shell needs quotes around a mapping: `floppy '{ name: Disk2, side: 1 }'`.

## 3. Replies

### 3.1 Plain (the default)

| First line | Then |
|---|---|
| `done` | nothing: the command was done and reads nothing |
| `done <n>` | n result lines |
| `refused: <reason>` | nothing |

Because of the count, a reader never takes a result line for the end of a
reply. A newline or `\r` inside a reason or a result line is written as a
space: each is one line.

### 3.2 JSON (`--json`)

One object on one line, the same three keys in this order for every
command:

```json
{"done":true,"result":{"ticks":120000,"seconds":0.500000000},"reason":null}
{"done":false,"result":null,"reason":"There is no topic foo."}
```

| Key | Value |
|---|---|
| `done` | `true` or `false` |
| `result` | what the command reads; `null` when it reads nothing or was refused |
| `reason` | the refusal, one line; `null` when done |

- No spaces. Objects keep their keys in the order this specification
  gives them.
- Strings are escaped `\"`, `\\`, `\n`, `\r`, `\t`; another control
  character as `\u00xx` (lower-case hex); the rest of UTF-8 as it is.
- A topic's or a session reply's value is converted as it is, with two
  rules: a real number is written with six decimals, rounded (`null` when
  not finite); bytes are a string `0x` followed by lower-case hex pairs.

### 3.3 Units and substitution

| What | Written as |
|---|---|
| Emulated time | `ticks` (an integer) and `seconds`: ticks divided by the machine's tick rate, with nine decimals, truncated, computed in integers |
| A duration in a command | a number and a unit: `2s`, `1.5s`, `500ms`; ticks = ⌈µs × rate / 10⁶⌉ |
| Brightness | three decimals, rounded |
| The run's save folder | `<save folder>`, in every reply, notices and log lines included |
| The run's folder | `<run folder>` |

No floating-point value reaches the output, so seconds and brightness
agree byte for byte between runs. Attach replaces no path.

## 4. Options, run folders and exit status

| Option | Meaning |
|---|---|
| `--product <name>` | The product, by its name (`Calc-U-59`) or product word (`c59`, `c1600`), in any case. Required |
| `--json` | Each reply as one JSON object (§3.2) |
| `--settings <file>` | Use this settings file. Without it the defaults are used, and no settings file is read or written |
| `--save-folder <dir>` | Use this save folder (made if missing). Without it an empty folder made for the run, removed at its end |
| `--output-folder <dir>` | Traces and screenshots go here (made if missing). Without it, to the save folder |
| `--clock <YYYY-MM-DDThh:mm:ss>` | The local date and time handed to a real-time clock at a build and at `sync-clock`; in between the clock runs in emulated time, so a program that sets it keeps its time. The year is 1970 or later |
| `--clock host` | The host's date and time, handed as the apps do: at a build, after each operation, at a `resume` and at `sync-clock`; a run is then not repeatable |
| `--help`, `-h` | The usage, every command's usage line, the products' `serve` options and the products of the build |
| `--dap <port>` | After `serve`, Calc-U-1600 only: §8 |

- Without `--clock` the instant is `2000-01-01T00:00:00`, a local date
  and time in no time zone, the same in every run.
- The run folder is `calcu-run-<n>-<m>` in the system's folder for
  temporary files. The default save folder (`save`), the home folder and
  the settings folder are inside it, so nothing of the user's is
  reached. It is removed at the end of the run.
- Bundled ROMs, bundled media and bundled presets are found as a
  frontend finds them.

| Exit status | When |
|---|---|
| 0 | Every command was done; `serve` at `quit` or at the end of its input |
| 1 | One-shot: the first refused command, after its reply; or a `check-preset` that named a failing file |
| 2 | The run cannot start; the message on standard error |

Messages of exit status 2:

| Case | Message |
|---|---|
| No `--product` | `calcu needs --product.` and the products of the build |
| No such product | `There is no product <name> in this build.` and the products of the build |
| No command | `calcu needs a command, or serve.` and the usage |
| Unknown option | `Unknown option <option>.` and the usage |
| Option without its value | `<option> needs a value.` |
| Bad `--clock` | `--clock takes YYYY-MM-DDThh:mm:ss or host: <value>` |
| A product option of another product | `<product name> has no option <option>.` |
| A `serve` option without `serve` | `<option> is an option of serve.` |
| A folder | `There is no folder for temporary files: …`, `Cannot make the run folder …`, `Cannot use the save folder <dir>.`, `Cannot use the output folder <dir>.` |
| ROM files | the text of the ROM lookup ([session-protocol.md, `roms-missing`](session-protocol.md#14-startup-and-machine-control)) |
| The start | `The machine was not built: <reason>` |

The products of the build are listed as `The products of this build:`
and one line `  <name> (<short name>)` each.

## 5. The common commands

The commands at the top level, in the order of `--help`. Each is the
session command or request named in [session-protocol.md](session-protocol.md);
a command that starts an operation replies when the operation has ended.
"—" in a result column: the reply is `done` and `result` is `null`.

A usage error is `refused: usage: <the command's usage>`.

### 5.1 Time, keys, display, topics

| Command | Plain result | `result` |
|---|---|---|
| `wait <duration>` | `ticks: <n>`, `seconds: <s>`: what passed; may exceed the duration by less than one instruction. When the debugger has the machine paused after it, also `run state: paused, owner <level>, reason <reason>` | `{ticks, seconds}`; paused: `{ticks, seconds, runState}` with the `run-state` value |
| `wait` | Until the machine is idle (settled: it waits for the user and nothing is busy, for the product's stretch), at most one hour of emulated time (the session's `wait-idle`): `ticks: <n>`, `seconds: <s>`, `hour reached: yes\|no` | `{ticks, seconds, hourReached}` |
| `wait-finished` | Until the machine is finished (idle, and its program has ended), at most one hour (the session's `wait-finished`): as `wait` | as `wait` |
| `idle-state` | the product's answers now: `takes keys: yes\|no`, `waits: yes\|no`, `ended: yes\|no` (waits, and the program has ended), `busy: <what>, …\|nothing` (the session's request `idle-state`) | `{takesKeys, waits, ended}` |
| `key-down <key ID>`, `key-up <key ID>` | — | — |
| `key <key ID> … [hold=<duration>]` | — (when the last key is up) | — |
| `type <text>` | — (when ENTER is up) | — |
| `display-text` | the rows; `indicators: <names>`; `not text: <n>` (dot matrix only); or the single line `display off` | `{on, rows: [..], indicators: [{name, brightness}], notText}` |
| `display` | dot matrix: one line per row of dots, each `1` or `0`; segments: `segments: <names>`, then `<position> <bits>` per position; then `indicators: <bits>`, `names: <names>`, `on: true\|false`, and for a display switched in parts `parts: <name> on\|off, …` | `{kind: "dots", rows: [[0,1,…]]}` or `{kind: "segments", segments: [..], positions: [[0,1,…]]}`, then `indicators: [0,1,…], indicatorNames: [..], on, parts: [{name, on}]` (empty for a display switched as a whole) |
| `state <topic>` | a state topic: its value as one line of JSON (`null` when never sent); a stream topic: `reset` when it started over since the last read, then each item appended since, one line of JSON each | `{topic, shape: "state", state}` or `{topic, shape: "stream", reset, items: [..]}` |
| `capabilities` | `capability <kind> <id> <owner>` per capability; `place <path> <peripheral>[:<variant>] …` or `place <path> empty` per place | `{capabilities: [{kind, id, owner}], places: [{place, plugged: ["<peripheral>[:<variant>]", …]}]}` |
| `ticks` | `ticks: <n>`, `seconds: <s>` | `{ticks, seconds}` |
| `audio <file>` | — | — |
| `state-save <file>` | — | — |
| `state-restore <file>` | — | — |
| `quit` | no reply | no reply |

- `on` is false while the display shows nothing, its dots and indicators
  all 0: it has no power or clock, or the program switched it off (the
  PC-1500's DISP; on the PC-1600 both display controllers, `&3E`).
- `parts` names the parts of a display that are switched on and off on
  their own, each with its own switch: the PC-1600's two display
  controllers, `IC2` (columns 0–63 and 128–155) and `IC3` (columns
  64–127 and the indicators). With one part off and the other on, `on`
  stays true and the part that is off shows dots that are 0:
  `parts: IC2 off, IC3 on`. A part's switch says nothing about power or
  the clock; `on` does.

- **Keys.** `key-down` and `key-up` take the key ID, the ID in any case,
  or another spelling of the key in any case; an unknown key is refused
  by the session (`Unknown key: <key>.`). `key` presses the keys in turn,
  each when the machine takes keys; the keys the product names (BREAK, the
  TI's R/S) wait for that only briefly. `hold=` holds each key exactly that
  long (`hold= takes a duration: <text>.`). `key` and `type` reply when their
  last key is up, not when the machine is idle; emulated time passes until then, at most one hour, else
  `refused: The keys were not taken within an hour.` `type` types the
  text and ENTER; a model without text typing refuses it.
- **Display.** `display-text` reads the `display` topic through the
  product's rule: for a dot matrix the rows of text the dots form (a cell
  that is no glyph is U+FFFD and counts in `notText`), for a segment
  display one string per position from left to right. The indicators are
  those that are on, in descriptor order; `brightness` is `null` where
  the descriptor gives none. `notText` is `null` unless the display is a
  dot matrix and on. `display` gives the output as the machine gives it.
- **Topics.** `state` takes the name of any topic of the session, also
  one that was declared and never sent. A name that is no topic:
  `refused: There is no topic <name>.` A stream read takes what it
  returns: the next read gives only what came after.
- **Owners** are written `component:<id>` or
  `peripheral:<id>@<place path>`. A place path is written with `/`
  between its steps (`a/jack`). The places are the machine's, then those
  of what is plugged in.
- **Sound.** `audio` writes all sound output from then on into a 16-bit
  mono PCM WAV file at the machine's sound rate, at every speed; the path
  is relative to the working directory. A second `audio` completes the
  first file and starts the new one. The file is complete at the end of
  the run. A machine without sound: `refused: This machine has no sound.`
- **Machine state.** `state-save` writes the full state of the machine
  and its plugged peripherals, `state-restore` continues from it. The
  file is a header, an empty line, then bytes internal to the product.
  The header:

  ```
  calcu-machine-state 1
  product: <product name>
  version: <product version>
  model: <model ID>
  variant: <variant ID, or ->
  plugged: <place path> <peripheral>[:<variant>]     (one line per plugged peripheral)
  ```

  A restore whose header differs from the machine's now is refused and
  changes nothing: `The file holds no machine state.` (no first line),
  `The file is of another product version, model, variant or set of
  plugged peripherals.` A machine without the kind:
  `This machine does not offer its machine state.` Files of the host
  (the save folder, a host drive's folder, a serial port file) are not
  part of the state.

### 5.2 Machine control

| Command | Session command |
|---|---|
| `model <name>` | `model {name}` |
| `variant <name>` | `variant {name}` |
| `attach <place> <peripheral>[:<variant>]` | `attach {place, peripheral, variant?}` |
| `detach <place>` | `detach {place}` |
| `reset` | `reset` |
| `cold-boot` | `cold-boot` |
| `power-cycle` | `power-cycle` |

Each replies `done` or `refused: <reason>` when its operation has ended.

### 5.3 Presets, speed, controls, steps

| Command | Plain result | `result` |
|---|---|---|
| `preset <path>` or `preset examples:<path>` | — | — |
| `check-preset <file> …` | `ok <file>` or `FAIL <reason>` per file | `[{file, ok, error}]`; `error` is `null` when ok |
| `preset-list` | `<bundle path> \| <model> \| startup\|example (<group>) \| <name>[ \| <description>]` per bundled preset | the `preset-list` value |
| `full-speed`, `regular-speed` | — | — |
| `log-level off\|info\|debug` | — | — |
| `set-control <peripheral> <control> on\|off` | — | — |
| `<step> [<value>]` | — or, for `elapsed-time report`, its line | — or `{text}` |

- `preset` and `check-preset` read a path relative to the working
  directory; `examples:<path>` is a bundled preset. `check-preset` reads
  and checks each file and builds nothing; its reply is `done`, and a
  failing file ends a one-shot run with exit status 1.
- `log-level` is the session's `log-level`: what the log tells, at `info`
  how a preset is carried out.
- `set-control` sets a switch, or presses (`on`) or lets up (`off`) a
  control of the form key. Peripheral and control are matched in any
  case.
- **Steps as commands.** Every preset step marked as a command is a
  command of the same name, sent as the session command `run-step
  {verb, value}`: `elapsed-time`, `expect`, `not-expect`, `screenshot`,
  `sync-clock`, the media steps of the product's kinds (`card`,
  `floppy`, `cassette`, `arm`), `eject`, `saveas`, and the product's
  own (`printer-trace`). The value of `expect`, `not-expect` and the
  other steps whose value is raw is the rest of the line as written.
  `screenshot` writes into the output folder.
- Steps that are no command of their own name: `key`, `type`, `wait`,
  `full-speed`, `regular-speed` and `cut-paper` have the commands above
  and below; `pause`, `resume`, `step`, `pause-on-start` and `trace` have
  the debugger's commands (§5.5); `debug-panel` has none.

### 5.4 Paper, media, open

| Command | Plain result | `result` |
|---|---|---|
| `cut-paper [<printer>]` | — | — |
| `paper text [<printer>]` | the text lines | `[..]` |
| `paper marks [<printer>]` | the mark lines | `[..]` |
| `paper png <path> [<printer>]` | — | `{path, width, height}` |
| `paper copy [<printer>]` | — | — |
| `paper cut [<printer>]` | — | — |
| `media` | the `media` topic as one line of JSON | `{topic: "media", shape: "state", state}` |
| `media import <file>` | — | — |
| `media export <kind> <name> <path>` | — | `{path}` |
| `media delete <kind> <name>` | — | — |
| `open <path>` | `<as> <file>[ <detail>]` | `{as, file, detail}` |

- `<printer>` is the owner's name in any case; without it, the printer
  that is plugged in. `cut-paper` cuts without a copy; `paper cut` copies,
  then cuts. A product without printers: `refused: <product name> has no
  printer.`
- `media` reads the topic again first. `media delete` is its own
  confirmation. `media export` writes the medium's file at `<path>` and
  its other files beside it. A product without kinds of medium:
  `refused: <product name> has no media.`
- `open` sends the file's bytes and its absolute path with the way
  `harness`; `<as>` is `preset`, `medium`, `file-kind` or `kept`.

### 5.5 The debugger

Each command is the debugger's session command or request with the
setter `harness`. A machine without a debug target refuses every one with
`The machine offers no debug target.`

| Command | Plain result | `result` |
|---|---|---|
| `pause [<level>]` | the run-state line | the `run-state` value |
| `resume` | the run-state line | the `run-state` value |
| `step`, `step <count>`, `step <level> [<count>]`, `step { level: program\|cpu, count: n }` | the run-state line | the `run-state` value |
| `step-over [<target>]`, `step-out [<target>]` | the run-state line | the `run-state` value |
| `pause-on-start <level>\|off` | — | — |
| `pause-at-start` | — | — |
| `trace <file>\|off` | — | — |
| `targets` | one block per target (below) | `{targets: [..]}` |
| `registers [<target>]` | `<name> = <value>` per register | `{<name>: "<value>", …}` |
| `set-register <target> <register> <value>` | — | — |
| `peek <target> <space> <address> [<count>] [bank=<b>] [vbank=<v>]` | `<address>: <value>\|unreadable\|none` per cell | `[{address, value}]`, both strings |
| `peek … file=<file>` | `<n> cells written to <file>` | `{count}` |
| `poke <target> <space> <address> <value>… [bank=<b>] [vbank=<v>]` | — | — |
| `break <target> <address> [<field>=<value>…]` | `breakpoint <id>` | `{id}` |
| `break off [<target> <address>]` | — | — |
| `break list` | `<id> <setter> <target> <address>` per breakpoint | `{entries: [..]}` |
| `watch <target> <space> <address> <count> read\|write\|both` | `watch <id>` | `{id}` |
| `watch off` | — | — |
| `watch list` | `<id> <setter> <target> space <space index> <address>+<count> <access>` per watch | `{entries: [..]}` |
| `history [<target>] [<count>]` | `<address>[ <<symbol>>][  <text>]  <register>=<value> …` per instruction | the `history` reply |
| `disassemble <target> <address> [<count>]` | `<address>[ <<symbol>>]  <text>` per instruction | the `disassemble` reply |
| `profile on`, `profile off` | — | — |
| `profile report [<count>]` | `<target> <address>[ <<symbol>>] <count>` per address, then `<n> different addresses` | the `profile-report` reply |
| `expect-register <target> <register> <value>` | — | — |
| `expect-memory <target> <space> <address> <value>… [bank=<b>] [vbank=<v>]` | — | — |
| `symbols <file> [<target>]` | `<n> symbols` | `{count}` |

**The run-state line:** `running[, armed <level>]`, or `paused, owner
<level>, reason <reason>[, target <target>][, hit <id> (<setter>)]…`.

**The `targets` block:**

```
<target>
  registers: <name>/<bits>[ (read-only)] …
  space <name>: <size> cells of <bits> bits[, the values last written][, banked <first>+<count>: <n> banks[, <m> vertical banks]]…
  code space: <space>
  offers:[ watches][ step-over-out][ decode][ history <n>]
  qualifier <field>: <value> … (now <value>)
```

- **Numbers** are read as decimal (a leading `-` allowed), or hex after
  `0x`, `$` or `&`. A word that is no number stands for a name loaded by
  `symbols`, wherever an address of that target is taken; an unknown name
  is refused. `history` and `disassemble` show the name of an address in
  `<…>`.
- **Output padding.** Addresses and values are written `0x` and lower-case
  hex digits, as many as the width needs: a register or cell of b bits
  gets ⌈b / 4⌉ digits; an address of a space of n cells gets the digits of
  the smallest multiple of 4 bits that holds n − 1 (4 digits when the
  space is unknown; the addresses of `break list` and `watch list` always
  4).
- `pause` without a level pauses `cpu`. `step` without a level steps
  `cpu`, with a count of 1; the count is 1 to 100000 (`step takes a count
  from 1 to 100000.`). `step-over` and `step-out` take one `cpu` step of
  that kind.
- **Pause and step without a wall clock.** A pause or step whose reply
  does not come at once lets emulated time pass in pieces of at most 1/60 s
  until it comes, at most one hour; then `refused: The machine did not
  pause.` A `wait` ends where the machine pauses, or at once on a paused
  machine.
- `break off` and `watch off` take away the harness's own only; `break
  off <target> <address>` those at that address.
- `peek … file=<file>`: the cells as bytes into the file, one byte per
  started 8 bits of a cell, little-endian, an unreadable cell as 0. A
  relative path is relative to the output folder of the run; attached, to
  the app's working directory.
- **`peek` and `poke` on a space of I/O.** A `peek` reads a cell
  without an effect, so a cell whose read has one (a device register)
  is `unreadable`. A `poke` is the CPU's own write, with its effects: on a
  port space an `OUT`, on a memory space with I/O in it a store, as the
  program's instruction would do it; the space of the values last
  written keeps it, as it keeps the program's. A space of the values last
  written is not written (`The cell <address> of <space> cannot be
  written.`). After a `poke` the `display` is read again, so a poke into
  display memory or a display port shows at once.
- `expect-register` is refused with `The register <name> is <value>.`,
  `expect-memory` at the first cell that differs with `The cell <address>
  of <space> is <value>.` (or `is unreadable.`, `holds none.`). A refused
  one ends a one-shot run with exit status 1.
- `state run-state` and `state trace` read the run state and the trace
  state.

## 6. Harness attach

An app in a development or test build opens its app session to the
protocol of `serve`, as a second listener without host services.

| Case | Rule |
|---|---|
| Builds | Only where `CALCU_ATTACH` is defined: POSIX development and test trees (the Qt6 apps), Debug builds of the Swift app on the iOS simulator. Never in a release build, a device build, or on Windows |
| Address | TCP, bound to `127.0.0.1` only. No authentication beyond that |
| Port | The app's launch argument `--attach-port N` or `--attach-port=N` (1 to 65535; another value is ignored), else 59059. `--attach-port 0` lets the system pick one (the Qt6 apps) |
| Bind fails | The app runs on; a line of the session's log: `Attach is not available on port <port>: <problem>.` |
| Clients | One at a time. A second gets the line `refused: one harness is already attached` and is closed; the first stays |
| `quit` | Ends the attach, not the app. A connection that closes without `quit` is taken as `quit` |
| Ending | The listener goes, with its pending replies; the keys held down through it come up |
| Suspended app | Every command line: `refused: the app is suspended` |
| `json on`, `json off` | A command of attach only, not of `serve`: switches the replies and message lines to JSON and back, its own reply included (`json on`: `{"done":true,"result":null,"reason":null}`, `json off`: `done`). Another argument: `refused: usage: json on|off`. A new attach starts plain |

**Commands.** Those of `serve` (§5) and the product's, on the app's
session and parts. Time is the app's: the harness never advances the
machine. `wait <duration>` replies when the app's own host calls have let
that much emulated time pass; the bare `wait` replies when the app's
machine is idle, `wait-finished` when it is finished (at most one hour). A command that starts an operation
replies when it ends; the lines after it wait in order.

**Lines from the app.** Each is one of:

| Line | Plain | JSON |
|---|---|---|
| A reply | as in `serve` (§3.1) | as in `serve` (§3.2) |
| A message | `message <topic> <text>` | `{"message":{"topic":"<topic>","value":<value>}}` |

- `<text>` is the payload as one line of JSON, with a newline in it
  written `\n` and a `\r` as `\r`; a stream's reset is the text `reset`
  (JSON value `"reset"`), then one line per appended item.
- At attach the listener is sent every state topic once and the stream
  topics with their history; then each change of a state topic and each
  stream topic's appended items, as they come. Replies, host-service asks
  and sound are not message lines.
- A message line never falls inside the result lines of a reply; a
  reader tells them apart by the first word: `done`, `refused:`,
  `message` (or `{"message":`).

**`calcu attach`.** Connects to `127.0.0.1:<port>` (`--port N`, else
59059), sends each line of its standard input and relays each reply
unchanged; message lines only with `--messages`. `--json` sends `json on`
first and drops its reply; a reply that is not the JSON `done` ends the
client (see the table below). Empty lines are not sent. It ends at `quit` or at the end of its
input, and sends `quit` then, with exit status 0.

| Case | Exit 2 with |
|---|---|
| `--product`, `--settings`, `--save-folder`, `--output-folder`, `--rom-folder`, `--roms`, `--clock` | `<option> does not apply to an attached harness: it works with the app's own product, settings and folders.` |
| Another argument | `usage: calcu attach [--port N] [--json] [--messages]` |
| No connection | `Cannot attach to 127.0.0.1:<port>: <system error>` |
| `--json`, and the reply to `json on` is not `{"done":true,…}` | `The app did not switch to JSON: <the reply>` (or `no reply`), after sending `quit` |
| Windows | `calcu attach is not available on this platform.` |

## 7. Python helper

`library/harness/python/calcu_serve.py` drives `calcu serve` over a pipe:
one command line per `send()`, its one reply back (the JSON object, or
`{done, lines, reason}` in plain form). Standard library only.

`library/harness/python/calcu_machine.py` builds on it: `Session`, a test
machine on `calcu serve` for pytest suites of any product, whose steps
(`key`, `type`, `wait`, `expect`, …) are queued and sent at the next read.
It is the product-neutral layer the Calc-U-1600 driver
(`products/calc-u-1600/tools/machine/`) builds on.

## 8. `serve --dap <port>` (Calc-U-1600)

A product's option of `serve`. Calc-U-1600's `--dap <port>` accepts a
debugger (DAP) on `127.0.0.1:<port>` beside the command lines of standard
input; both are taken in the order they come. While the debugger has the
machine running and no line waits, emulated time passes unpaced in pieces
of 1/60 s; there is still no wall clock. A scenario that pauses at a
breakpoint repeats exactly; one that pauses by hand does not. A port the
server cannot take ends the run with exit status 2 and the reason on
standard error.

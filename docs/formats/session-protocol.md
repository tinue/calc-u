# The session protocol

The messages between an app session and its listeners: the frontends,
the harness, and a translating host (the Swift bridge, the Web worker).
It says which kinds of message there are, in which order they come, and
for each library area the topics, commands and requests with the shapes
of their arguments and values.

Terms follow `docs/GLOSSARY.md`. The session is in `library/sessions/`:
`Messages.hpp` (the messages), `AppSession.cpp` (the order),
`LibraryTopics.hpp` (its own names), `HostServices.hpp`. Each area
defines its names in the file the tables name.

Product topics, commands and requests carry the product's prefix and are
not listed here: `c59.*` in `products/calc-u-59/docs/developer/Debug-API.md`,
`c1600.*` in `products/calc-u-1600/docs/developer/App-Layer.md`.

## 1. Values

Arguments of commands and requests, and what a translating host makes of
a payload, are values (`sessions/Value.hpp`): null, boolean, integer
(64 bits), real, string, bytes, array, object. Objects keep their members
in the order given; the shapes below give them in that order. In a text
form (logs, the harness) bytes are a string `0x` followed by hex pairs.

In the tables, `{a, b?}` is an object with the member `a` and the
optional member `b`; `x | y` is one of two shapes; "—" is no value.

A payload has a **kind** (`display`, `busy`, …; a product's with its
prefix) and a value. A state is sent again only when it is not equal to
the last one sent.

## 2. Messages

| Kind | Fields | To |
|---|---|---|
| State | `topic`, `state` (a payload) | every listener |
| Stream | `topic`, `reset` (the stream started over first), `appended` (payloads in order) | every listener |
| Reply | `call`, `done`, `reason` (when refused), `result` (a payload, or none) | only the listener that sent the command or request |
| Ask | `ask` (its call ID), `service`, `args` | only the first listener that offers the service |
| Sound start | `sampleRate` | the audio sink, or a recorder |
| Sound samples | `mono` (16-bit samples) | the audio sink, or a recorder |
| Sound stop | — | the audio sink |

A listener sends **commands** (`{name, args}`) and **requests**
(`{name, args}`), each with one reply, and **answers** to an ask
(`{given, result, reason}`).

- Call IDs are per session and increasing; commands, requests and asks
  share them.
- A command is carried out at once. Its reply says done, with a result
  where the command gives one, or refused with one English sentence; a
  refused command changes nothing. A command that starts an operation
  replies when the operation ends; one that defers (`key`, `type`,
  `pause`, `step`) replies later, once, and the session is not busy
  meanwhile.
- A request reads and changes nothing; its reply comes in the same flush.
- An unknown name: `Unknown command: <name>.`, `Unknown request: <name>.`

### 2.1 One flush

A flush happens at the end of each host call, each command, each request
and each answer. Its order:

1. the parts' last updates of their topics, and `busy`;
2. sound messages of that host call, and a sound stop when the machine
   changed;
3. the state topics that changed, in the order they were declared;
4. the stream topics with something appended or a reset, in that order;
5. the replies;
6. the asks.

A listener therefore has the new state when the reply arrives. The
declared order is `display`, `busy`, `notices`, `log`, `log-level`, then the topics of
each part in the order of the parts.

### 2.2 A new listener

When a listener starts to listen, what is pending goes first to the
listeners there are. The new one is then sent, at once:

1. the last sent state of every state topic that has one, once;
2. for each stream topic kept since its last reset, one stream message
   with `reset` and everything since that reset. A stream without
   history (`notices`, `paper-copies`) sends nothing.

A listener states the host services it offers when it starts to listen.
Every message goes to the sink on the session's thread; whether the sink
acts at once or queues is the sink's. After the session is closed no
message goes out.

## 3. Host services and sound

`sessions/HostServices.hpp`. A service is asked of the first listener
that offers it, with one ask and one answer. When no listener offers it,
nothing is asked.

| Name | Asked by the library with | Answer |
|---|---|---|
| `audio-sink` | (not asked: the listener gets the sound) | — |
| `clipboard-text` | — | — |
| `clipboard-image` | `paper.copy`, `paper.cut {copy: true}`: `{bytes (PNG), text?, name, dotsPerInch}` | `{textOnly?}` |
| `file-picker` | `media.import` without paths: `{accept (suffixes, comma-separated), multiple: true}` | an array of paths |
| `folder-picker` | — | — |
| `keep-file` | `media.export`: `{name, bytes, mediaType}` per file; a trace kept in memory: `{name, bytes, mediaType, kind: "trace"}`; `screenshot` without a folder: `{name, bytes}` | — |
| `open-url` | (answered by the frontends for their own use) | — |
| `haptics` | — | — |

**Sound.**

| Listener | Gets |
|---|---|
| A recorder (states that it records sound) | every sample at every speed; a sound start before the first samples of each machine |
| The audio sink (the first listener that offers `audio-sink` and is no recorder) | the sound of regular speed outside an operation only |

The audio sink is opened (sound start, then 40 ms of silence) at the
first sound that is not silence, and closed (sound stop) after one second
of silence or when the machine changes. A sample of magnitude 8 or less is
silence.

## 4. Sessions

`sessions/LibraryTopics.hpp`, `sessions/AppSession.cpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `display` | state | — | `{on, frame, indicators: [{name, on, brightness}]}`; `frame`: `{kind: "segments", positions: [{lit: [bool], brightness, segmentBrightness: [..]}]}` or `{kind: "dots", width, height, dots (bytes, row-major, 0 or 1)}`; a brightness is a real or null |
| `busy` | state | — | `{operationRuns, speed: "regular" \| "full"}` |
| `notices` | stream, no history | — | `{sentence}`: one English sentence for the user |
| `log` | stream, since reset | — | `{text}` |
| `log-level` | state | — | `{level: "off" \| "info" \| "debug"}` |
| `suspend` | command | — | — |
| `continue` | command | — | — |
| `speed` | command | `{full}` | — |
| `key-down`, `key-up` | command | `{key}` (a key ID) | — |
| `log-level` | command | `{level}`: `off`, `info` or `debug` | — |
| `start` | command | `{model?, preset?}` | — (§14) |
| `ticks` | request | — | `{ticks, seconds}` (seconds a real) |
| `capabilities` | request | — | `{capabilities: [{kind, id, owner}], plugged: {byPlace: [plug], byConnector: [{owner, connector, plugged: [plug]}]}, places: [..]}`; `owner` is `{component}` or `{peripheral, place}`; a plug is `{place, peripheral, variant}`; a place is an array of its steps |
| `topics` | request | — | `{topics: [{name, stream}]}`, every declared topic in declared order |

- `suspend` tells the parts (a part writes what it must keep), then the
  session's host calls do nothing until `continue`. Both are done when
  already so.
- `speed` sets the speed outside an operation; while an operation runs it
  is done and changes nothing.
- `log-level` sets what `log` tells; it starts at `off`. At `off` the log
  holds what a step gives on purpose (`elapsed-time: report`), the
  library's and the products' events (a medium saved, why a preset
  stopped, a file left out); `info` adds how a preset is carried out (the
  machine and its peripherals, each item as it is done, its end);
  `debug` adds a product's own detail (Calc-U-59: each write to the
  registers and the memory). Another level: `The log level is off, info or debug.`
- `key-down` and `key-up` call the machine at the current tick; an
  unknown key: `Unknown key: <key>.`; arguments without `key`: `The key
  command needs key: <key ID>.`

**Fixed sentences** of the session:

| Sentence | When |
|---|---|
| `Another operation is running.` | a command that starts an operation while one runs |
| `Stopped.` | the reply of an operation that a later command stopped (`model`, `reset`, `cold-boot`) |
| `No machine is running.` | a command or request that needs a machine, without one |
| `The operation failed.` | an operation that failed without a reason; its reason is also a notice |
| `The operation did not finish.` | a session hosted in virtual time whose operation did not end in one go |
| `The command failed.` | a command whose handler failed without a reason |
| `The speed command needs full: true or false.` | `speed` without `full` |
| `The log level is off, info or debug.` | `log-level` with another level |

## 5. Control state

`control/ControlState.hpp`, `control/ControlState.cpp`.

| Name | Shape | Value |
|---|---|---|
| `control-state` | state | the object below |

Its members, in this order:

| Key | Value |
|---|---|
| `models` | `[{id, available, reason?}]` |
| `model` | the running model's ID |
| `variants` | `[{id, available, reason?}]`: the running model's |
| `variant` | its variant's ID, or null |
| `places` | `[{place, connector, pinOut, available, reason?, empty: {available, reason?}, fitting: [choice]}]`; a choice is `{id, name, plugged, variant?, variants: [{id, available, reason?}], available, reason?}` |
| `mediaPlaces` | `[{id, owner?: {place, peripheral}, kind, passage: "holding" \| "pass-through", sides: [..], insert: {available, reason?}}]` |
| `panels` | `[{kind, owner?: {place, peripheral}}]`; `paper` is the one panel kind the library knows |
| `controls` | `[{place, peripheral, id, form: "key" \| "switch", on}]` |
| `actions` | `[{command, available, reason?}]`, in the order `reset`, `cold-boot`, `load-preset`, `open` |
| `product` | `{<prefixed name>: value, …}`: the product's own entries, unread by the library |

- `reason` is present only where `available` is false: one English
  sentence, the first that applies.
- A place is written as its path with `/` between the steps (`a/jack`).
- A choice's `variant`: the one that runs when it is plugged in, else the
  one an attach uses. Its `available`: not plugged in, its attach (a
  replace when the place is full); plugged in, its detach.
- `controls` lists every control of every plugged-in peripheral, in the
  order of the place walk and each owner's in its descriptor's order;
  `on` is a key down or a switch on.

## 6. Typing

`typing/TypingTopics.hpp`, `typing/HostKey.hpp`, `typing/TypingPart.cpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `host-key-down` | command | `{physical, named, produced, carried, shift, control, alt, command, meta, mac-host, auto-repeat}` | — |
| `host-key-up` | command | `{physical}` | — |
| `host-text` | command | `{text}` | — |
| `screen-key-down` | command | `{key, pointer}` | — |
| `screen-key-up` | command | `{key, pointer}` | — |
| `focus-lost` | command | — | — |
| `paste` | command | `{text}` | — |
| `key` | command, deferred | `{names: [..]}` or `{names: "<names parted by spaces>"}` | — |
| `type` | command, deferred | `{text}` | — |
| `held-keys` | state | — | `{live: [key IDs]}`: the keys live input holds down |
| `typing` | request | — | `{idle, waiting}` |

- `host-key-down`: `physical` is the host's own identity of the key, which
  pairs a key-down with its key-up; `named` is one of `escape`,
  `backspace`, `return`, `enter`, `tab`, `delete`, `up`, `down`, `left`,
  `right`, `f1` to `f6`, `scroll-lock`, `insert`, `home`, `end`,
  `page-up`, `page-down`, or empty; `produced` and `carried` are code
  points (0 for none).
- `host-key-down`, `host-text`, `paste` and `screen-key-down` are dropped
  (done, nothing typed) while an operation runs.
- `key` and `type` reply when their last key is up. `key` takes key IDs or
  other spellings; refused: `The key command needs one or more key IDs.`,
  `Unknown key: <name>.` `type` exists only in a product that types text;
  refused: `Unknown command: type.` (a model without text typing), `This
  model types no text.`, `No key types the character "<c>".`

## 7. Presets

`presets/PresetsPart.hpp`, `presets/PresetsPart.cpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `load-preset` | command, operation | `{ref, buildOnly?}` or `{content, name?, buildOnly?}` | — |
| `run-step` | command | `{verb, value}` | — or `{text}` |
| `wait-idle` | command, operation | — | — |
| `check-preset` | request | `{ref}` or `{content, name?}` | `{ok: true}` |
| `preset-list` | request | — | `{entries: [{bundlePath, model, name, description, kind: "startup" \| "example", group, files: [..]}]}` |

- `ref` is a path, or `examples:<path>` for a bundled preset. `content` is
  the preset's text; `name` names it in sentences (else `the preset`).
- `load-preset` reads and checks the whole file first: a read error is
  the refusal and a notice. Its operation builds and carries out the
  preset; with `buildOnly` it only builds. At its end the speed is
  regular.
- `run-step` carries out one step on the running machine. `value` is the
  step's value as text: raw for a raw step, else read as a scalar, flow
  mapping or flow sequence. A step that needs no running machine and is
  over when it took effect is done at once, with `{text}` where it gives
  a line (`elapsed-time report`); the others are an operation. Refused
  while an operation runs, without a machine, or `The machine is paused.`
- `wait-idle` waits until the machine is idle, at most one hour of
  emulated time; on a paused machine: `The machine is paused.`
- `preset-list` without bundled presets: `This product bundles no presets
  of format 2 yet.`; an entry that cannot be read is left out and its
  sentence goes to `log`.

## 8. Peripherals

`peripherals/PeripheralsPart.hpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `set-control` | command | `{peripheral, place?, control, on}` | — |

The peripheral and control are matched in any case. A switch is set; a
control of the form key is pressed (`on: true`) or let up. Refused: `No
<peripheral> is plugged in.`, `The <peripheral> has no controls.`, `The
<peripheral> has no control <control>.`

## 9. Media

`media/MediaTopics.hpp`, `media/MediaPart.cpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `media` | state | — | `{kinds: [kind], places: [place]}` |
| `media.refresh` | command | — | — |
| `media.choose` | command | `{place, entry}` | — |
| `media.eject` | command | `{place}` | — |
| `media.turn` | command | `{place}` | — |
| `media.arm` | command | `{place, entry}` | — |
| `media.feed` | command | `{place, side}` | — |
| `media.name-and-save` | command | `{place, name, protect?, confirmed?}` | — |
| `media.start-new` | command | `{place, template, name, confirmed?}` | — |
| `media.open-file` | command | `{name, place?, path?, bytes?, upload?, confirmed?}` | — |
| `media.import` | command | `{paths?}` | — |
| `media.export` | command | `{kind, name}` | — |
| `media.delete` | command | `{kind, name, confirmed?}` | — |

- A kind: `{kind, label, suffix, templates: [item], instances: [item]}`;
  an item: `{name, description, fileName, origin, template, inPlace?}`; `origin` is `bundled`, `implicit`, `saved` or `opened`.
- A place: `{place, kind, slot, passThrough, sides: [..], holds?, passing?,
  waits, nameAndSave, askWriteProtect, picker: [entry]}`. `place` is the
  place word. `holds`: `{name, template, unsaved, writeProtected, side?}`.
  `passing`: the side passing through now. `waits`: the machine waits for
  a medium now.
- A picker entry: `{group, id, label, short, canChoose, chosen, tooltip?, hint?}`;
  `group` is `empty`, `bundled-template`, `user-template`, `instance`,
  `module` or `held`; `id` is what `media.choose` and `media.arm` take back;
  `chosen` marks the one entry of what the place holds (the `empty` entry
  while it holds nothing); a medium held that no other entry lists (a file
  opened from where it lies, a tape started under a name before its first
  write) comes last as a `held` entry, never `canChoose`;
  `short` is what a picker shows of the chosen entry where room is short:
  a `module` entry's module ID (`ML`), `—` for `empty`, else `label`.
- `media.eject` is `media.choose` with the empty entry. `media.import`
  without `paths` asks the `file-picker`; refused when nobody offers one
  (`No file picker is offered.`). `media.export` asks `keep-file`, the
  medium's file first; refused when nobody keeps a file (`Nobody keeps a
  file here.`).

**Questions.** A refusal whose reason starts with `asks: ` is a question,
not a refusal: the frontend asks the user the rest of the sentence and, on
yes, sends the same command again with `confirmed: true`. It comes from
`media.name-and-save`, `media.start-new`, `media.open-file`,
`media.delete` and `open` (a medium of that name exists already; a
delete).

## 10. Paper

`paper/PaperTopics.hpp`, `paper/PaperPart.cpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `papers` | state | — | `[{printer, owner, description}]`, the product's printers in declared order |
| `paper <owner>` | stream, since reset | — | `{continuesStroke, marks: [..], lines: [{text, x, y}], reach: [from, to]}`; a reset says the paper was emptied |
| `paper-copies` | stream, no history | — | `{owner, outcome: "copied" \| "text-only" \| "failed", reason}` |
| `paper.copy` | command | `{owner?}` | — |
| `paper.cut` | command | `{owner?, copy}` | — |
| `paper.text` | request | `{owner?}` | `{lines: [..]}` |
| `paper.marks` | request | `{owner?}` | `{lines: [..]}` |
| `paper.picture` | request | `{owner?}` | `{bytes (PNG), dotsPerInch, width, height}` |

- `owner` is the owner's name in any case; without it, the printer that is
  plugged in.
- `description`: `{unit: [numerator, denominator], width, printableLeft,
  printableWidth, form: "dots" \| "pen-strokes", dotDiameter,
  darknessGreys: [..], strokeWidth, pictureDotsPerInch, textLayer}`.
  Coordinates are integers in ten-thousandths of the printer's unit.
- `marks` in the order made: a run of dots is one flat array `[x, y,
  darkness, …]`; a stroke is `{colour: "#rrggbb", path: [x, y, …]}`.
- `paper.copy` asks `clipboard-image`; its outcome comes on `paper-copies`.
  `paper.cut` with `copy: true` copies, then cuts whatever the answer;
  with `copy: false` it only cuts, also an empty paper.
- Sentences: `No printer is plugged in.`, `The paper is empty.`, `No
  frontend offers the clipboard.`, `The <owner> has no text layer.`,
  `Unknown printer: <name>. Printers: <owners>.`

## 11. Debugger

`debugger/RunState.hpp`, `debugger/DebuggerPart.cpp`,
`debugger/TraceSession.cpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `run-state` | state | — | `{paused, owner, reason, target, hits: [{id, setter}], armed, pauseAtStart}` |
| `trace` | state | — | `{running, file}` |
| `pause` | command, may defer | `{level?}` | the run state |
| `resume` | command | — | the run state |
| `step` | command, may defer | `{level?, count?, target?, kind?: "in" \| "over" \| "out"}` | the run state |
| `pause-on-start` | command | `{level}` or `{level: "off"}` or `{off: true}` | — |
| `pause-at-start` | command | — | — |
| `break` | command | `{setter?, target, address, qualifier?: {<field>: value}}` | `{id}` |
| `break-off` | command | `{setter?, id?}` | — |
| `watch` | command | `{setter?, target, space, address, count, access: "read" \| "write" \| "both"}` | `{id}` |
| `watch-off` | command | `{setter?, id?}` | — |
| `set-register` | command | `{target, register, value}` | — |
| `poke` | command | `{target, space, address, values: [..], bank?, vbank?}` | — |
| `trace` | command | `{file}` or `{button: true}` or `{off: true}` | — |
| `profile` | command | `{on}` | — |
| `symbols` | command | `{file, target?, bytes?}` | `{count}` |
| `targets` | request | — | `{targets: [{name, registers: [{name, bits, writable}], spaces: [{name, cellBits, size, writtenView, banked: [{first, count, banks, vbanks}]}], codeSpace, offers: {watches, stepOverOut, decode, history, qualifier: [{name, values, now}]}}]}` |
| `registers` | request | `{target?}` | `{target, registers: [{name, bits, value}]}` |
| `peek` | request | `{target, space, address, count?, bank?, vbank?}` | `{target, space, cellBits, cells: [{address, value} \| {address, unreadable: true} \| {address, none: true}]}` |
| `history` | request | `{target?, count?}` | `{target, registers: [names], entries: [{address, registers: [..], text?, symbol?}]}` |
| `disassemble` | request | `{target, address, count?}` | `{target, instructions: [{address, text, cells, symbol?}]}` |
| `breakpoints` | request | `{setter?}` | `{entries: [{id, setter, target, address, qualifier: [..]}]}` |
| `watches` | request | `{setter?}` | `{entries: [{id, setter, target, space (index), address, count, access}]}` |
| `profile-report` | request | `{count?}` | `{entries: [{target, address, count, symbol?}], distinct}` |

- **Run state.** `owner` is the level that paused (null while running);
  `reason` is `pause`, `step`, `breakpoint`, `watch`, `pause-on-start` or
  `entry`; `target` the current debug target; `hits` the breakpoints or
  watches that stopped it; `armed` the level of a pause-on-start.
- **Pause and step.** `level` defaults to `cpu`, in any case; `count` to
  1, from 1 to 100000. A pause or step that does not land at once defers
  its reply until it lands. A step is refused on a machine that is not
  paused (`The machine is not paused.`); `over` and `out` where the target
  offers no stepping over and out.
- **Setters.** A breakpoint or watch belongs to its setter (`harness`
  without one; `dap`). `break-off` and `watch-off` take the setter's own
  only, all of them without `id`. `breakpoints` and `watches` list all
  setters' without `setter`.
- **Numbers and names.** An address, value, count or bank is an integer,
  or a string in decimal or hex (`0x`, `$`, `&`). An address may be a
  name loaded by `symbols` for that target. `symbols` reads `bytes`, else
  the file.
- **Trace file names.** A name without a folder (no `/` or `\`), written
  into the trace folder and replacing a file of that name; `{button:
  true}` uses the product's name for a trace started by a button. One
  trace at a time: a new one ends the running one. A trace ends at the
  size limit (`[trace] max-file-size-mb`, 50 MB when it is 0 or less) with the notice `The
  trace <name> reached the size limit of <n> MB and ended.`, or at a new
  machine (log: `The trace <name> ended: a new machine was built.`).
  Without a trace folder it is kept in memory where a listener offers
  `keep-file`, and handed over when it ends; otherwise `There is no
  folder for traces.`
- `profile {on: true}` starts counting afresh; `profile-report` gives the
  `count` (default 20) most frequent addresses.
- A machine without a debug target refuses the commands and requests
  with `The machine offers no debug target.`, except `trace` (refused by
  the product's recorder instead), `break-off`, `watch-off`,
  `breakpoints` and `watches`.

## 12. Opening

`opening/OpenRequest.hpp`, `opening/OpenPart.cpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `open` | command | `{files: [{name, path?, bytes?}], way?, upload?, confirmed?}` | `{as, file, detail}` |
| `open-would-take` | request | `{name, size?}` | `{takes}` |

- `way` is `drop` (the default), `open-with`, `start`, `dialog` or
  `harness`. Several files come only from a Web file dialog. `upload`:
  the host cannot write the file back. `bytes` missing: the host could not
  read the file. A file needs a name or a path.
- The reply (`OpenReply`): `as` is `preset`, `medium` (`detail`: the place
  word, or `imported`), `file-kind` (`detail`: the kind's name) or `kept`.
- While an operation runs or before a machine exists, the file is
  **kept** (reply `as: kept`) and opened after the operation; a second
  one replaces it (a log line says so). A refusal of a kept file, or a
  question, comes as a notice: `<file> was not opened: <reason>`.
- A refusal is the reply `done: false` with its sentence, and the same
  sentence as a notice; a question (`asks: `, §9) is no notice.
- `open-would-take` answers, during a drag, whether a drop of that file
  would be taken.

## 13. Shell and settings

`shell/SettingsPart.hpp`, `shell/Inputs.hpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `settings` | state | — | `{home, rows: [{section, key, kind, value, default, choices: [..], min?, max?, inUse, isDefault}]}` |
| `set-setting` | command | `{section, key, value}` (`value` null: the default) | — |
| `remember-folder` | command | `{kind, folder}` | — |

- `rows`: every declared key but `[window]` and `[startup] last-model`,
  and the keys a frontend shows besides. `kind` is `text`, `path`,
  `boolean`, `integer`, `choice`, `model-or-last`, `model-name` or
  `folder` (a `[folders]` key). `value` and `default` are text.
- `set-setting` is carried out also while an operation runs. A value the
  key does not take is refused with the reason; a file that cannot be
  written gives the notice `The setting holds until you quit; it was
  <problem>.` and keeps the value for the run.
- `remember-folder`: the folder a file dialog of that kind was last used
  in.

**What the shell reads and sends.** The shell's models take these inputs
as values: `control-state`, `busy`, `run-state`, `roms-missing`,
`settings`, `media` (topics) and the reply of `preset-list`. They send
`model`, `variant`, `attach`, `detach`, `reset`, `cold-boot`,
`load-preset`, `resume`, `paste`, `media.import`, `screen-key-down`,
`screen-key-up`, `speed`, `set-setting`, `remember-folder`.

## 14. Startup and machine control

`startup/StartupPart.hpp`, `startup/StartupPart.cpp`,
`roms/Lookup.cpp`.

| Name | Shape | Arguments | Value |
|---|---|---|---|
| `start` | command, operation | `{model?, preset?}` | — |
| `model` | command, operation; ends a running one | `{name}` | — |
| `variant` | command, operation | `{name}` | — |
| `attach` | command, operation | `{place, peripheral, variant?}` | — |
| `detach` | command, operation | `{place}` | — |
| `reset` | command, operation; ends a running one | — | — |
| `cold-boot` | command, operation; ends a running one | — | — |
| `power-cycle` | command, operation | — | — |
| `roms-missing` | state | — | `{text}` |

- `start` is taken once (`The product has started already.`): it builds
  the startup model (or `model`; an unknown one gives the notice `There
  is no model "<name>"; the startup model is used.`) with its startup
  preset (or `preset`), and runs it to key readiness. A startup preset
  that cannot be loaded gives the notice `The startup preset of the
  <model> could not be loaded: <reason>. The <model> starts as its
  built-in default machine.`; one that fails while it runs `The startup
  preset of the <model> stopped: <reason>.`
- `model` and `variant` take a name in any case; the same one again is
  done at once. Refused: `There is no model "<name>".`, `The <model> has
  no variant "<name>".`
- `attach`: `place` is a place path with `/`; the same peripheral and
  variant again is done at once; into a full place that takes one, it
  replaces. `detach` of an empty place is done at once.
- `model`, `reset` and `cold-boot` stop a running operation when they
  start their own (its reply: `Stopped.`). `variant`, `attach`, `detach`,
  `power-cycle` and `start` are refused while one runs.
- Every operation ends at key readiness.

**`roms-missing`** is set only when the full set of ROM files was not
found; then no machine is built and `start` is refused with the same
text:

```
<n> ROM files are missing or bad:          (or: 1 ROM file is missing or bad:)
  <file>: <problem>[ (<detail>)][; needed by <who>[ (variant <v>)], …]
Looked in: <where>
The full set of ROM files is needed; no machine starts without it. In a local build, run the product's roms/fetch_roms.sh to fetch them, then build again.
```

| Problem | Text |
|---|---|
| missing | `missing` |
| wrong amount (size, or what the reader yields) | `wrong size` |
| unreadable | `cannot be read` |
| the product's check | `fails its check` |
| the catalogue | `listed twice in different forms` |

## 15. The harness's own

`harness/StatePart.hpp`: `machine-state.save` (command; value: the
state's bytes with their header) and `machine-state.restore {bytes}`
(command). The header and the refusals are in
[harness.md §5.1](harness.md#51-time-keys-display-topics).

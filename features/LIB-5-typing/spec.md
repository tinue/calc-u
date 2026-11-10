# LIB-5: Typing

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (key down and key up by key ID, key readiness,
  emulated time), LIB-3 (the machine session, commands, topics, busy),
  LIB-11 (the harness, for the commands `key` and `type`).
- Contracts and decisions: `docs/contracts/typing.md` (T-1 to T-4, I-1
  to I-3, Q-2, Q-3), `docs/contracts/frontend.md` (FE-4),
  `docs/contracts/presets.md` (PR-5, for `key` and `type`);
  `docs/behaviour-changes.md` rows 59-5, 59-13, 1600-8.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`.

What the three status columns mean for this feature:
- **Library:** the typing engine turns host keys, on-screen keys and
  text into key down and key up on a machine, by a typing profile, shown
  with a machine and a profile made for the tests.
- **Calc-U-59, Calc-U-1600:** every frontend of the product and the
  harness send their input through the typing engine, with the product's
  typing profile and keyboard map per model; the product's own key
  sequencers and queues are gone.

Words used here: **live input** is what a user does now, a host key or
an on-screen key; **typed text** is a string the engine types by itself
(a paste, a `type` step); a **press** is one key going down and later
up.

## User Stories
- As a user, I want every key I press to arrive, however fast I type and
  whether the machine runs at regular speed or at full speed.
- As a user, I want a key I hold down to stay down, so that the
  machine's own auto-repeat and its own key combinations work as on the
  real device.
- As a user of a Sharp machine, I want to paste a line of text and to
  type accented letters from my own keyboard.
- As the author of a preset or a harness script, I want to name keys and
  to type text, and to get the same result every time.
- As the developer, I want one engine for all of this, so that the
  rules about holds and waits are not written per product and per
  frontend.

## Out of Scope
- Which point on the screen is which key: the hit test, how a pressed
  key is drawn, and the gesture "hold the display" (the frontends;
  LIB-14, C59-8; the open point "Calculator view" in
  `features/INDEX.md`).
- Which surface of a frontend has the host keyboard, and focus
  (`frontend.md` FE-4: the frontends, as today). This feature takes
  "focus lost" as an input.
- The click and the haptics on a key press (the frontends' own).
- The preset loader and the other steps, the wait of a preset for the
  machine to be idle, and a preset that fails (owner: LIB-6). This
  feature gives `key` and `type` their meaning.
- Key readiness itself (LIB-1 AC-42; C59-1, C1600-1).
- Dropping the keys of a medium or a card corner tap (LIB-7, C59-7).
- More host keys for Calc-U-59, a host keyboard on iOS, Paste Text for
  Calc-U-59 (`Briefing.md` §1.3, §2: no new functions).

## Acceptance Criteria

### Library

Presses
- [ ] **AC-1** — Given a press, then the key is down for at least the
  minimum hold of the model's typing profile, counted in emulated time;
  a key-up that comes earlier is carried out when the minimum is
  reached, never dropped (`typing.md` T-1).
- [ ] **AC-2** — Given live input finds no other press waiting and no
  key held by the engine, then its key goes down at once, whatever the
  machine is doing; given it finds presses waiting, then it waits its
  turn (OQ-1).
- [ ] **AC-3** — Given a press waits its turn, then it starts when the
  key before it is up and, where the typing profile says so, the machine
  reports key readiness (LIB-1 AC-42) or the profile's cadence has
  passed (`typing.md` T-2).
- [ ] **AC-4** — Given a host key is held down by the user, when its
  press starts, then the key stays down until the user lets go; given
  the user has already let go when its turn comes, then it is typed as
  one press of the minimum hold. Auto-repeat events of the host are
  ignored (`typing.md`, responsibilities).
- [ ] **AC-5** — Given the user holds two keys at once, then both are
  down on the machine together, each from its own key-down to its own
  key-up; the engine itself never makes two keys overlap that the user
  did not hold together (`typing.md` T-3, I-2).
- [ ] **AC-6** — Given the same input at the same tick counts in two
  runs, then the same keys go down and up at the same tick counts, at
  regular speed, at full speed and without a wall clock (`typing.md`
  I-1, I-3).

Host keys
- [ ] **AC-7** — Given a host key event, then the frontend sends what
  the host says: which key, which modifiers, and the character it
  produced if it produced one; the engine turns it into a key, a
  sequence of keys or nothing by the keyboard map of the running model,
  which the product supplies. No frontend holds a keyboard map.
- [ ] **AC-8** — Given a host key that maps to a sequence of keys (a
  second function, a shifted character, an accent), then the keys are
  pressed one after the other, each as a press of its own, and the
  user's key-up of that host key does nothing.
- [ ] **AC-9** — Given live input arrives while an earlier sequence or
  typed text still has presses waiting, then what is waiting is dropped
  and the new input takes its turn; a part of a sequence that must be
  finished to leave the machine in order (the closing key of an accent)
  is still typed (both products today).
- [ ] **AC-10** — Given the frontend says that it lost the focus, then
  every key the user holds is released, after its minimum hold, and
  presses that wait are dropped.

Typed text
- [ ] **AC-11** — Given a text and a typing profile that maps characters
  to keys, then the engine types it character by character, each as the
  keys the profile gives, with the cadence the profile gives for typed
  text.
- [ ] **AC-12** — Given Paste Text, then only the text before the first
  line break is typed, characters that have no key are left out, and
  ENTER is not pressed (`typing.md`, responsibilities).
- [ ] **AC-13** — Given the machine has typing support (a capability of
  this feature) that reports latches, then typed text takes them into
  account as the profile says: a key that is latched is not pressed
  again, or is released first.
- [ ] **AC-14** — Given the machine has typing support that reports
  prompt readiness, then a `type` starts its line only when the machine
  reports it.

Commands and steps
- [ ] **AC-15** — Given the command or step `key` with one or more
  names, then each named key is pressed in turn as in AC-1 and AC-3; a
  name is a key ID of the model or another name the typing profile gives
  for a key or for a sequence of keys; a name the model does not have
  refuses the command (`presets.md` PR-5).
- [ ] **AC-16** — Given the command or step `type` with a text, on a
  model whose profile maps characters, then the text is typed and ENTER
  is pressed after it; a character without a key refuses the command and
  nothing of the text is typed; on a model without such a profile the
  command does not exist (`presets.md` PR-5).
- [ ] **AC-17** — Given `key` or `type`, then the reply comes when the
  last key is up.

While an operation runs
- [ ] **AC-18** — Given an operation runs, then live input is dropped;
  a key that is still held when the operation ends is not pressed then
  (`typing.md` T-4).

What is shown
- [ ] **AC-19** — Given the keys that live input holds down on the
  machine, then a state topic lists them by key ID, so that a frontend
  can draw them pressed; the keys of typed text and of steps are not in
  it (Calc-U-59 today, `Shared/KeySequencer.cpp:150`).

### Calc-U-59
- [ ] **AC-20** — Given a TI model, then its typing profile gives: a
  minimum hold of 100 ms of emulated time; presses that wait start at
  key readiness; no characters (`typing.md` T-1, fit table; today 100 ms
  of wall time, `Shared/KeySequencer.hpp:80`).
- [ ] **AC-21** — Given the keyboard map of Calc-U-59, then it maps the
  26 keys it maps today and no others, by these rules
  (`Shared/KeyboardMap.cpp:10-65`):

  | Host | Key or keys |
  |---|---|
  | `a` to `e` | `A` to `E`; with Shift: `2nd`, then the letter |
  | `x`; with Shift | `*`; `2nd`, then `*` |
  | `0` to `9` | the digit |
  | `.` and `,` | `.` |
  | `+` `-` `*` `/` | `+` `-` `*` `/` |
  | `=`, Return, Enter on the number pad | `=` |
  | `(` `)` | `(` `)` |
  | `<` | `+/-` |
  | `>` | `EE` |
  | Escape | `CLR` |

  With Command, Control or Option held, nothing is mapped. For a letter
  the key itself counts and Shift means `2nd`; for anything else the
  character the layout produced counts.
- [ ] **AC-22** — Given the frontends of Calc-U-59, then the host
  keyboard reaches the machine in the Swift frontend on macOS, in Qt6
  and in the Web frontend, and not on iOS, as today
  (`App/Views/KeyboardView.swift:243-324`,
  `Qt6/app/KeyboardWidget.cpp:123-172`,
  `Web/src/PlayCalculator.jsx:235-259`).
- [ ] **AC-23** — Given a second key is pressed while a first is held,
  then both are down (AC-5). Today the first is released
  (`Shared/KeySequencer.cpp:256-270`; behaviour change 59-13).
- [ ] **AC-24** — Given an on-screen key in any frontend of Calc-U-59,
  then it goes through the typing engine like a host key, minimum hold
  included. Today the Swift and Qt6 frontends press the key matrix
  directly, without a minimum hold
  (`App/EmulatorViewModel.swift:249-250`,
  `Qt6/app/EmulatorController.cpp:57-58`).
- [ ] **AC-25** — Given a key pressed on a running program (R/S to stop
  it), then it goes down at once (AC-2), as today.
- [ ] **AC-26** — Given the names a `key` takes on a TI model, then they
  are the 45 key IDs of C59-1 AC-9 and these 43 names of second
  functions, each of which stands for `2nd` followed by its key; a `2nd`
  written before such a name is pressed once, not twice; names are
  matched without regard to case
  (`tools/headless/ti59.py:44-54`; `presets.md` PR-5 as amended):

  | Row | Names, left to right |
  |---|---|
  | 1 | `A'` `B'` `C'` `D'` `E'` |
  | 2 | — — `log` `CP` `2ndCLR` |
  | 3 | `Pgm` `P>R` `sin` `cos` `tan` |
  | 4 | `Ins` `CMs` `Exc` `Prd` `Ind` |
  | 5 | `Del` `Eng` `Fix` `Int` `\|x\|` |
  | 6 | `Pause` `x=t` `Nop` `Op` `Deg` |
  | 7 | `Lbl` `x>=t` `S+` `mean` `Rad` |
  | 8 | `StFlg` `IfFlg` `D.MS` `pi` `Grad` |
  | 9 | `Write` `Dsz` `Adv` `Prt` `List` |

- [ ] **AC-27** — Given what the tests of Calc-U-59 check today about
  keys, then it holds with the engine, with emulated time in place of
  wall time: a key of a sequence waits for key readiness and is held
  exactly the minimum; a key-up right after the key-down is deferred to
  the minimum; a Shift binding gives two presses and a new key drops the
  second; the keyboard map's rules
  (`tests/shared_cooperative_tests.cpp:111-186`,
  `tests/shared_tables_tests.cpp:42-61`).

### Calc-U-1600
- [ ] **AC-28** — Given a Sharp model, then its typing profile gives: a
  minimum hold of 2 frames, 1/30 second of emulated time; for typed
  text a cadence of 4 frames down and 4 frames up, and on the PC-1600 6
  frames more after a SHIFT or KBII press; and the characters of AC-30
  (`typing.md` T-1; `Shared/MachineSession.cpp:42`,
  `Core/KeyPaste.hpp:82-98`).
- [ ] **AC-29** — Given the keyboard map of Calc-U-1600, then it is the
  one of today for every model, as `docs/Keyboard-Mapping.md` gives it:
  the named keys (Backspace, F1 to F6, Scroll Lock and Insert, Home,
  End, Page Up, Page Down, Return and Enter, the cursor keys, Delete,
  Tab), with their Shift forms on the PC-1600, and the characters of
  AC-30; with the host's command modifiers nothing is mapped, and
  Option or AltGr still type (`Shared/HostKeyMap.cpp:54-118`). It is the
  same in the Qt6 and the Web frontend.
- [ ] **AC-30** — Given a character, then it is typed as today: a letter
  as its key; a digit and `+ - = * / ( ) .` as its key; space as
  `space`; the 13 second legends as `shift` and their key; on the
  PC-1600 also the digit-row legends as `shift` and their key, and an
  accented letter as `kbii`, `shift` where needed, its key, `kbii`
  (`Core/SharpShiftedSymbols.hpp:17-63`,
  `Core/PC1600/PC1600TypedInput.hpp:26-111`). In typed text a lower-case
  letter is `shift` and its key, so that the case is kept
  (`Core/PC1500/PC1500TypedInput.hpp:25-42`).
- [ ] **AC-31** — Given a host key held down on any Sharp model, then
  its key stays down until it is released, so that the ROM's own
  auto-repeat works for every key (behaviour change 1600-8). Today that
  holds on the PC-1600 and for the cursor keys of the PC-1500; other
  keys of the PC-1500 go through a queue of 16 with a fixed cadence of 4
  frames down and 4 up, which drops a key when it is full
  (`Shared/MachineSession.cpp:371-407`,
  `Core/PC1500/PC1500Machine.cpp:342-377`).
- [ ] **AC-32** — Given the PC-1500 and PC-1500A, then how their ROM
  takes keys is researched as part of this feature, and they report key
  readiness from it (`typing.md` Q-3; C1600-1 AC-21); the fixed cadence
  and the queue of 16 then go. Until that is done, AC-31 holds for them
  as it does today.
- [ ] **AC-33** — Given the SHIFT key of the host is tapped alone,
  pressed and let go within 400 ms with nothing else in between, then
  the machine's `shift` key is pressed once
  (`Qt6/app/MainWindow.cpp:71, 609-614, 713-720`,
  `Web/src/CalcU1600.jsx:15, 321, 335-339`).
- [ ] **AC-34** — Given Paste Text on a Sharp machine, then AC-12 holds;
  on the PC-1600 the SHIFT and KBII latches are read first: with SHIFT
  latched it is released first, with KBII latched an accented character
  is left out (`Core/KeyPaste.cpp:5-26`, `Core/KeyPaste.hpp:65-79`). A
  second paste while one is typed is added after it
  (`Shared/MachineSession.cpp:419-428`).
- [ ] **AC-35** — Given typing support of the Sharp machines, then it
  reports prompt readiness and, on the PC-1600, the SHIFT and KBII
  latches, as they are found today
  (`Core/PC1500/PC1500TypedInput.hpp:51-56`,
  `Core/PC1600/PC1600TypedInput.hpp:118-137`).
- [ ] **AC-36** — Given an operation runs in Calc-U-1600, then AC-18
  holds in both frontends. Today the Qt6 frontend holds input back
  during an operation, and the Web frontend lets keys reach the machine
  (`Qt6/app/SyncOperations.cpp:59-72`, `Web/src/CalcU1600.jsx:307-331`;
  OQ-4).
- [ ] **AC-37** — Given what the tests of Calc-U-1600 check today about
  keys, then it holds with the engine: an instant press and release is
  one key; a double tap is two; a fast sequence comes out in order; the
  rules of the keyboard map per model; the cadence of typed text; the
  KBII sequences and latches; a paste types the first line and no ENTER
  (`Shared/tests/shared_machine_session_tests.cpp:96-190`,
  `Shared/tests/shared_host_key_map_tests.cpp:24-90`,
  `Core/tests/key_paste_tests.cpp:45-310`).

## Edge Cases
- **EC-1** — Given a key-up for a key that the engine does not hold for
  that host key, then nothing happens. Today Calc-U-59 releases whatever
  key is held (`Shared/KeySequencer.cpp:280-294`).
- **EC-2** — Given a host key whose character has no key on this model,
  then nothing is typed and no press waits.
- **EC-3** — Given a press waits for key readiness and the machine does
  not report it (a program that never stops), then it waits without
  limit until it is dropped by AC-9, AC-10, a reset or a new machine;
  both products wait without limit in this case today.
- **EC-4** — Given a text that the input method of the host commits as a
  whole (an accent composed on the host), then each of its characters is
  typed as a press of the minimum hold (Calc-U-1600 Qt6 today,
  `Qt6/app/MainWindow.cpp:632-644`).
- **EC-5** — Given the machine is reset, power-cycled or cold booted, or
  replaced, then every key the engine holds is up afterwards and nothing
  waits.
- **EC-6** — Given two host keys that map to the same key of the machine
  are held together, then the key is down until the last of them is let
  go (Calc-U-1600 Web today for two pointers,
  `Web/src/Faceplate.jsx:93-103`).

## Technical Requirements (optional)
- The engine never reads the wall clock: every hold, gap and cadence is
  counted in the machine's emulated time (`typing.md` I-1; ADR 0009).
- The one rule that uses wall time is the tap of AC-33, which is about
  the user's hand, not about the machine; the frontend measures it.
- The research of AC-32 is recorded in the PC-1500 research repository
  before the cadence is removed.

## Open Questions
- [ ] **OQ-1** — AC-2: live input that finds nothing waiting goes down at
  once. `typing.md` T-2 says key readiness "gates only the start of the
  next queued press". Read strictly, a key pressed on a running TI
  program would wait for a readiness that does not come, and R/S could
  not stop the program. Today a single key goes down at once in both
  products. *Recommendation:* as in AC-2; it is today's behaviour and
  what T-2 means by "queued".
- [ ] **OQ-2** — The letters of the Calc-U-59 keyboard map in the Web
  frontend follow the position of the key on a US keyboard today
  (`Web/src/PlayCalculator.jsx:34-43`), while Swift and Qt6 follow the
  letter the key carries. With one map in the library there is one rule
  (AC-7, AC-21). *Recommendation:* the letter the key carries, in every
  frontend.
- [ ] **OQ-3** — Focus loss (AC-10): Calc-U-59 releases the held key at
  once today, without its minimum hold (`Shared/KeySequencer.cpp:296-306`).
  *Recommendation:* keep the minimum, so that a press is never too short
  for the ROM.
- [ ] **OQ-4** — AC-36 is a change for Calc-U-1600 that
  `docs/behaviour-changes.md` does not list: keys pressed while a preset
  or a load runs are dropped. Row 59-5 says it for Calc-U-59.
  *Recommendation:* add a row for Calc-U-1600.
- [ ] **OQ-5** — `type` and a character without a key (AC-16): a preset
  fails on it today, and a paste leaves it out. Should the harness
  command `type` fail like the step? *Recommendation:* yes; the command
  and the step are one.
- [ ] **OQ-6** — The other names of Sharp keys in `key` (AC-15): presets
  accept `ent` for `enter` and `break` for `on` today.
  *Recommendation:* `break` stays as another name of `on`; `ent` goes;
  names are matched without regard to case on every model.
- [ ] **OQ-7** — The name `2ndCLR` in AC-26 for the second function of
  `CLR`, and `Adv` for the second function of `.`, which is not the ADV
  key of the PC-100C. *Recommendation:* keep both as today's tool
  spells them; the PC-100C's key is a control, named with its owner.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Minimum holds are emulated time, per model in the typing profile: TI 100 ms, Sharp 2 frames | `typing.md` T-1 | 2026-11-10 |
| The engine holds presses, not taps; a held key stays down until key-up; the host's auto-repeat is ignored; readiness gates only the start of a press that waits | `typing.md` T-2 | 2026-11-10 |
| Key readiness is reported by the machine, worded as "ready to accept a key press" | `typing.md` T-2 as amended (user) | 2026-11-10 |
| Keys held together are fed to the matrix as they are | `typing.md` T-3; behaviour change 59-13 | 2026-11-10 |
| Live input that arrives while an operation runs is dropped | `typing.md` T-4; behaviour change 59-5 | 2026-11-10 |
| Paste Text never presses ENTER; a host key stops a running paste | `typing.md`, responsibilities (adopted from Calc-U-1600) | 2026-11-10 |
| The PC-1500's key handling is researched when this feature is built; its fixed cadence then goes | `typing.md` Q-3 (user); behaviour change 1600-8 | 2026-11-10 |
| `key` takes one or more names; `type` types a line and ENTER | `presets.md` PR-5 | 2026-11-10 |
| On a TI model a second-function name stands for `2nd` and its key, and a `2nd` before it is not pressed twice | `presets.md` PR-5 as amended (user) | 2026-11-10 |
| Proposed in this spec, for the gate: live input goes down at once when nothing waits (AC-2); a state topic lists the keys live input holds (AC-19); on-screen keys of Calc-U-59 get the minimum hold (AC-24); a key-up acts only on its own key (EC-1); the 43 names of AC-26 and the place of this list, here instead of C59-2 | Today's behaviour in both products for AC-2; the frontends draw pressed keys from such a state today; one engine means one path; the index gives `key` and `type` to this feature | 2026-11-10 |

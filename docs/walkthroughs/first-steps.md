# First steps of phase 6

Phase 5 (`docs/Process.md`; prompt `prompts/phase-5.md`, step 7). What
the specs, the ADRs and the ledger leave to a spike or to a check on a
running product, and what the preset pass could not settle on paper
(`docs/walkthroughs/presets.md`, class C; below `presets.md` means
that file, and a contract is named with `contracts/`). None of it was
carried out in phase 5. The
migration plan starts with these steps, before anything is built on
what they test.

Four kinds:
- **Spike:** something new is tried in a throwaway build.
- **Check:** a product is run at its pin and what it does is noted.
- **Record:** reference data is taken from a product at its pin, to
  compare against later.
- **Research:** a ROM is read until a fact is found.

The order puts first what most specs depend on. Each step says what is
done, which criteria wait for it, and what changes if it turns out the
other way. Where a step waits for a ruling on a finding of
`presets.md`, it names it.

## Before anything else

### 1. The pin of Calc-U-1600 (check)
- **What:** when `v0.8.1` is tagged, its tree is compared with
  `947b850`, the tree of `dev-0.8.1` @ `b4ea8c8` (`docs/Process.md`,
  phase 1).
- **Depends on it:** every `path:line` of Calc-U-1600 in the specs, the
  inventories and `presets.md`.
- **If it differs:** the difference is recorded as a delta against the
  pin, and each spec that cites a changed file is read again.

### 2. Golden runs of both cores (record)
- **What:** scripts of calls on each machine (keys at tick counts,
  attaches, advances), recorded from the code as it is imported, before
  a core is changed, with the display output, the sound (Sharp), the
  printed lines and dots (PC-100C) and the memory at the end (C59-1,
  C1600-1 and C59-6, Technical Requirements). They cover at least:
  switching on, a small calculation or BASIC line, an error, a reset, a
  power cycle and a cold boot after memory was filled, and one run for
  each peripheral and for two Solid State ROM modules.
- **Depends on it:** C59-1 AC-4, AC-5; C1600-1 AC-4; C59-6 AC-3, AC-7,
  AC-9, AC-10; and steps 10 to 12 and C-7 below, which are each one more
  golden run.
- **If a run cannot be recorded** (a state the old interface cannot
  reach): the run is cut to what can be, and the criterion that names
  it says so.

## Spikes

### 3. The Swift session on a background thread (spike)
- **What:** the app session of the Swift frontend on one thread of its
  own, called about 60 times a second, with the bridge as a queue of
  messages in both directions; tried with a TI-59, and, as ADR 0009
  allows, with a PC-1600 on iOS, since that is the machine the decision
  was made for (ADR 0015).
- **Depends on it:** C59-5 AC-4 to AC-6; LIB-3's Calc-U-59 column
  (`features/INDEX.md`: complete only with C59-5); C59-8 as a whole.
- **If it does not hold** (the display stutters, messages arrive out of
  order, SwiftUI cannot be fed from the queue at 60 Hz): ADR 0015 is
  reopened with the user; the alternative it names is the main thread.

### 4. Harness attach in the iOS simulator (spike)
- **What:** a Debug build of the Swift frontend in a simulator started
  through Xcode's Device Hub opens its session on a TCP port bound to
  localhost, and `calcu` on the Mac attaches (ADR 0033; C59-5 AC-10 to
  AC-14). Tried too: whether the Xcode plugin the user installed helps
  (its MCP server did not connect in phase 4b and phase 5:
  "Connection closed"); several simulators at once; and how an
  XCUITest, which runs inside the simulator, reaches the attached
  session and learns that a load it started with a tap has ended
  (`presets.md` B-21).
- **Depends on it:** C59-5 AC-10 (the default port is "set after the
  phase 5 spike", OQ-1), AC-11 to AC-14, EC-1 to EC-4; C59-8 AC-17 and
  AC-21 (the UI tests and the screenshot tests lose
  `keystroke-playback-status`, `--UITesting` and the hidden R/S button
  and get the attached harness); ADR 0032 for every GUI scenario of the
  Swift frontend. If B-22 is ruled as recommended, the same mechanism
  is built for Qt6.
- **If it does not hold** (the simulator's sandbox refuses the port, or
  a test cannot reach it): C59-8 AC-21 keeps the three aids until
  another way is found, and ADR 0033 is amended with the user.

## Checks on Calc-U-59 at `v2.3.0`

### 5. Key readiness and "idle" on a TI machine (check)
- **What:** on each model, with and without the PC-100C, a running
  program (a loop, a `Pause`, a printing program, a library program):
  at which addresses the processor is when 2.3.0 takes a key
  (`Shared/MachineModelInfo.hpp:59-68`), whether that state shows for a
  moment while a program runs, and whether 100 ms of emulated time
  without a break tell a stopped machine from a running one (LIB-6
  AC-36, OQ-1). In the same run: whether 100 ms of emulated hold are
  enough wherever 100 ms of wall time were (`contracts/typing.md` Q-2;
  LIB-5 AC-25), and whether, with FREEZE ON START armed, the freeze lands
  while the key that starts the program is still down (`presets.md`
  C-1, on `examples/screentest/test_cdebug.ti59`).
- **Depends on it:** LIB-6 AC-15, AC-21, AC-36 (its Calc-U-59 column
  "waits for this"); C59-1 AC-18; LIB-5 AC-20, AC-25; C59-2 AC-1, AC-7
  and AC-11, which all act "when the machine is idle"; `presets.md`
  B-1 to B-3.
- **If no stretch tells the two apart:** idle becomes a state of its
  own in the machine interface (LIB-1, C59-1), and LIB-6 AC-36 is
  rewritten. **If 100 ms of hold are too short somewhere:** the engine
  holds the key until the ROM has sampled it (LIB-5, Technical
  Requirements). **If the freeze lands inside the hold:** the rule of
  B-2 is needed as written; if not, it stays as a safeguard.

### 6. Reference runs of the 57 TI examples (record)
- **What:** for each example of `examples/`, what its 2.3.0 file leaves
  behind: display, program steps, registers, partition, paper, and the
  cards it wrote (C59-2 AC-11). The tools AC-11 names cannot play most
  of them (`presets.md` B-10), so the first task is the recorder: a
  driver on 2.3.0's own `SessionController` in its cooperative mode,
  with a clock the driver supplies. Recorded before any file is
  upgraded and before core code is touched (C59-2, Technical
  Requirements).
- **Depends on it:** C59-2 AC-10, AC-11; C59-7 AC-30 (the four card
  examples); LIB-6 AC-43.
- **Waits for:** B-10 (the recorder and where a run ends), B-12
  (`example-brief.ti59`), B-1 (when a key step ends, which decides what
  the version 2 files look like).
- **If an example's two runs differ:** the upgrade rule that produced
  the difference is a finding; the spec is changed with the user, not
  the reference.

### 7. Reset and the partition (check, by a golden run)
- **What:** on a TI-59 and a TI-58 with a partition other than the
  ROM's default: what today's Reset leaves (C59-1 OQ-5).
- **Depends on it:** C59-1 AC-6; ledger row 59-22, which says "today,
  as read from the code".
- **If 2.3.0 already keeps the partition:** row 59-22 goes; AC-6
  stands.

### 8. Print instructions without a printer (check, by a golden run)
- **What:** a program that executes print instructions, run without
  the PC-100C: display output and tick counts (C59-6 OQ-2).
- **Depends on it:** C59-6 AC-7; the design of the PC-100C as a
  peripheral (the busy time must pass with the printer moved out of the
  processor).
- **If the ROM never executes a print instruction without a printer:**
  AC-7 holds trivially and the design is free.

### 9. An empty module slot (check, by a golden run)
- **What:** `2nd Pgm 01` with no module, on the code as imported, with
  the slot giving what an empty slot gives (C59-6 OQ-1).
- **Depends on it:** C59-6 AC-10 (the display blinks as for an error,
  from the real machine); LIB-18's built-in default machine of the
  TI-58 and TI-58C.
- **If the emulated ROM does something else:** the slot's model of "no
  module" is wrong, not the criterion; it is corrected until the run
  shows the blink.

### 10. An empty card side (check)
- **What:** a side without a bank inserted while the calculator waits
  for a key: how the ROM's error shows, and that the pass ends by
  itself (C59-7 OQ-3).
- **Depends on it:** C59-7 EC-2; ledger row 59-20.
- **If the ROM never gives the card back:** the user decided there is
  no fallback by time (C59-7 OQ-8); the finding goes back to the user.

### 11. The TI-58C's constant memory (check)
- **What:** (a) a TI-58C that is filled, quit and started again in
  2.3.0: do the partition and the display format come back (C59-3
  OQ-2). (b) What H00 to H03 hold after a cold boot, and whether a
  program a preset wrote outlives a Reset when they were left as the
  ROM's start-up made them (`presets.md` C-6, B-28, on
  `examples/historic/repartition.ti58c`).
- **Depends on it:** C59-3 AC-2, AC-8; C59-2 EC-3, AC-1; C59-1 AC-6;
  ledger row 59-15 ("to be confirmed in phase 6").
- **If (a) shows that 2.3.0 already restores them:** row 59-15 is no
  change and goes. **If (b) shows that zeroed H registers make the ROM
  clear the memory:** the three criteria say "as the ROM's start-up
  leaves it" (B-28).

### 12. The Mac app in the background (check)
- **What:** whether macOS reports the background for a window that is
  only covered, while 2.3.0 holds off App Nap (C59-5 OQ-5).
- **Depends on it:** C59-5 AC-8, AC-9, which are worded exactly after
  it.
- **If a covered window is suspended today:** AC-9 says so instead.

## Checks on Calc-U-1600 at `b4ea8c8`

### 13. Reference runs of the 32 Sharp examples (record)
- **What:** for each example, what its version 1 file leaves at the end
  of its load: display as text, BASIC program area, placed machine
  code, paper (C1600-4 AC-16), and, if B-11 is ruled as recommended,
  the media. Made in a copy of `examples/`, because one example
  rewrites two tracked files and one mounts a tracked folder. Noted in
  the same runs, from the log each load writes:
  - whether a bare `wait:` in MODE 1 and a `type` in RESERVE mode find
    the command loop, or run into their bound (`presets.md` C-2, on
    `plotter/lissajou-ce150.pc1600` and `startup/default-pc1600.pc1600`);
  - how long the load of `machine-code/Calculat.pc1600` lasts, whose
    program never returns to BASIC (C-3);
  - whether `dwx/DiskWorks.pc1600` writes into `examples/dwx/S3` (C-4);
  - whether the start-up of a ROM writes into a battery-backed module,
    by the module's content counter after the boot (C-5; needed only if
    B-5 keeps LIB-7 AC-25 for templates).
- **Depends on it:** C1600-4 AC-15, AC-16; LIB-6 AC-45, AC-46; LIB-5
  AC-14, AC-17, AC-28; C1600-2 AC-13.
- **Waits for:** B-4 (if `key` and `type` stay as today, the end of a
  load does not move and the runs compare like with like), B-11.
- **If C-2 shows a bound was hit:** C1600-1 says how a PC-1600 in that
  mode reports prompt readiness. **If C-3 shows the hour:** the user
  rules on a shorter bound for an auto-run program, or the load ends
  after the start command. **If C-4 shows a write:** AC-16 says that
  such runs use a copy of the folder.

### 14. The long-run pacing tests on the old code (record)
- **What:** per product two tests exist before its pacer is replaced
  and are run on the old code first: one with a scripted wall clock,
  one of ten minutes on the real clock (LIB-17, Technical
  Requirements).
- **Depends on it:** LIB-17 AC-18, AC-21 (the same long-run rate before
  and after); the user's condition in `contracts/frontend.md` SP-1. For
  Calc-U-1600 the measured old drift is the expected difference (LIB-17
  decision log).
- **If the old code does not meet the bound of AC-1:** the bound is the
  old code's measured one for the "before" test, and the difference is
  stated in the test.

### 15. The PC-1500's key handling (research)
- **What:** the key scan loop of the PC-1500 and PC-1500A ROMs is read
  until a signal for key readiness is found or shown not to exist
  (`contracts/typing.md` Q-3, `contracts/debugger.md` Q-1); the result is recorded in the
  PC-1500 research repository first (LIB-5, Technical Requirements).
  With it, by a golden run: which mode a PC-1500 and a PC-1500A are in
  after a cold boot (`presets.md` C-7, B-29).
- **Depends on it:** LIB-5 AC-31, AC-32; C1600-1 AC-21; LIB-10 AC-38
  (`pause-on-start: cpu` on the PC-1500); ledger row 1600-8; C1600-4
  AC-4.
- **If a signal is found:** AC-31 holds for every key of the PC-1500
  and AC-32 is struck. **If none is found:** AC-32 stays, row 1600-8 is
  corrected, and `pause-on-start: cpu` is not offered on the PC-1500.
  Either way the spec is changed then, with the user (LIB-5 OQ-10).

## Not first steps

- **Questions that stay with an SC61860 product:** ROM versions as
  variants (`contracts/machine.md` Q-1), a peripheral or RAM card change
  on a PC-1403 or PC-1350 (`contracts/peripherals.md` Q-2), their key
  handling (`contracts/typing.md` Q-1). The scenarios that would have answered them in
  phase 5 were dropped (`docs/Process.md`); `docs/contracts/README.md`
  still names "a phase 5 walkthrough" as the other place for the first
  two.
- **The power state** (`contracts/capabilities.md` Q-1) was answered in phase 4b
  (its decision log, 2026-11-10; LIB-1 decision log); the table in
  `docs/contracts/README.md` still lists it as open for a phase 5
  walkthrough.
- **The invariants** that LIB-1, LIB-2, LIB-3 and C59-5 leave to "review
  and the phase 5 walkthroughs" (no wall time, thread or lock in a
  machine; no ROM data in a core; nothing above a core reaching a
  peripheral; the bridge only translating): the preset pass looked at
  presets and did not check them. They are checked in review, and
  where the spec says so by a check in the build.
- **Work of phase 6 that is not a check:** the upgrade of the shipped
  and hosted presets (`contracts/presets.md` Q-1), a Python helper for
  `calcu serve` (`contracts/harness.md` Q-2), and saying in
  `ti59-research` and in Calc-U-59's documents that the printer timings
  were measured on a PC-100A (`Briefing.md`, C59-6).

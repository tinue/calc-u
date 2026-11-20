# Proposal: idle as three questions, one place per machine

A working document: today it is X, proposed Y. Once agreed, the code, the
glossary and the formats change, and this file goes.

## Today

One word, "idle", answers three questions, and each answer is put together
in several places:

| Question | Today | Asked by |
|---|---|---|
| A. Does the machine take a key now? | `Machine::isKeyReady()`, in each core | the typing engine (a scripted press waits for it), start-up, Reset, power cycle, Cold boot (`untilKeyReady`), the debugger's level "start", DAP |
| B. Has it finished what it was given? | the product's `IdleRule`: its own readiness held for a stretch (TI 100 ms, Sharp 20 frames), plus extras | preset items, harness `wait`, `let-it-finish`, the driver's `key`/`type`/`wait` |
| C. Does it wait, so that time need not race? | `MachineSession::waitsForInput()`: A, or a media place waits | pacing: at full speed, Calc-U-59 slows to real time while the machine waits; Calc-U-1600 never does |

Beside these: `TIRunProbe::runIdle` (A, no program running, printer not
busy) for the `c59` print listing.

What goes wrong:

- **The extras reach only some askers.** The PC-1600's floppy motor and
  direct file command are in B only; C (pacing) does not see them. The TI's
  card wait is written twice (B and C); its printer check only in `runIdle`.
- **The machines answer A differently.** The PC-1600's A is "the key matrix
  was read in this frame", which is true while a program polls INKEY$ and
  while LOAD watches for BREAK. The PC-1500's A is "in the ROM's key wait
  WAIT_4_KB", which is false while a program polls INKEY$. Measured, with
  `10 A$=INKEY$:GOTO 10`, `RUN`, `wait`: the PC-1600 is idle after 0.33 s,
  the PC-1500A not within the hour.
- **Every key waits for A, BREAK too.** On a PC-1500A running `10 GOTO 10`,
  a scripted `key break` took 13 s of real time, and the program was still
  running afterwards (BUSY, no BREAK).
- **B was patched twice in one day.** The direct file command first made
  SRAMINIT, a program started by CALL that waits for Y or N, look busy: an
  hour per key. A program waiting for a key is idle in every sense a test
  needs; "a program runs" is not the line.

## The line: an ordinary key

The questions become sharp when A means **an ordinary key**: one the ROM or
a program reads as input. Watching for BREAK alone is not taking keys.

| State | A: takes an ordinary key | Waiting (nothing else happens until a key) |
|---|---|---|
| BASIC prompt, TI scan loop | yes | yes |
| `INPUT`, a program polling INKEY$, SRAMINIT asking Y/N | yes | yes |
| A direct LOAD, SAVE, COPY; a program that never reads the keyboard | no (BREAK only) | no |
| A TI Write waiting for a card | no | yes: it waits for a medium |
| At a prompt, the printer still printing, the plotter moving, the floppy motor on | yes | no |

A program that polls INKEY$ while it animates is waiting by this test,
though things still happen. No signal tells the two apart; a test gives it
seconds (`wait(2)`, `wait_for`), as today.

## Proposed

### Four predicates, defined once per machine

- **A, `takesKeys`**: the machine takes an ordinary key now. In the core,
  one sample, no stretch: `Machine::isKeyReady()` keeps its name and gets
  exactly this meaning.
- **W, `waits`**: nothing happens until the user acts: A or a medium is
  awaited, and nothing is busy (printer, plotter, floppy motor, a running
  transfer). One sample.
- **Settled**: W held for the machine's stretch, within a bound. The
  library's `IdleWatch`, as today, on W instead of the product's mixed rule.
  The ordinary steps wait for it.
- **B, `finished`**: the program has ended: no BASIC program and no
  machine code started by CALL runs (Sharp), no program runs (TI), and the
  machine is settled. Asked only where "ended" is meant.

C is W: pacing asks W per slice.

| Case | A | W | settled | B |
|---|---|---|---|---|
| BASIC prompt, TI scan loop | yes | yes | yes | yes |
| `INPUT`; a PRINT stop (PC-1500 without a WAIT statement; PC-1600 after a plain `WAIT`); INKEY$ polling; SRAMINIT asking Y/N | yes | yes | yes | no |
| a direct LOAD, SAVE, COPY | no | no | no | no |
| `10 GOTO 10`; a TI program running | no | no | no | no |
| a TI Write waiting for a card | no | yes | yes | no |
| the prompt while the printer prints or the plotter moves | yes | no | no | no |

The product defines A, W's busy list, the stretch, the sample and the key
rules in one place per model: a table-like struct in the app layer
(`SharpWaitRules`, `TIWaitRules`) over the core's signals and capabilities.
The library takes it as today's `IdleRule` is taken, with fields
`takesKeys`, `busy` (named conditions, so a refusal or a log can say which
one held), `awaitsMedium`, `stretch`, `sample`, `keys`.

### Who asks what

| Asker | Asks |
|---|---|
| a scripted press, before it goes down | A, unless the key is in the list below |
| after a scripted key or line (preset items, driver `key`, `type`) | settled |
| harness `wait`, driver `wait()` without seconds | settled |
| `let-it-finish`; a new explicit wait "until the program has ended" (harness, driver, preset) | B |
| start-up, Reset, power cycle, Cold boot | A (the ROM's start is over at key readiness) |
| debugger level "start" | A ending (a program starts when the machine stops taking keys) |
| pacing at full speed | W: while it holds, time runs at real time, so polling adds a user's time and no more |
| the TI print listing | B (the printer is in W's busy list, the program state in B; `runIdle` goes) |

### Keys that do not wait

A list per machine of keys that go down at once, held for a time of their
own:

| Machine | Keys | Hold |
|---|---|---|
| PC-1500, PC-1600 | `on` (`break`) | the ROM latches it: the minimum hold; the PC-1500's CLOAD reads the level (hold not established) |
| TI-58/58C/59 | `R/S`, `GTO` (between program steps), `RST` (library programs), `ADV` and `.` (Adv) | until the ROM has sampled it (today's sample rule), at most a bound above a Pause (432 ms) |

A step can give the hold itself: `key("gto", hold=0.5)` in the driver,
`key gto hold=500ms` in the harness, `- key: { id: gto, hold: 0.5 }` in a
preset. `key-down`/`key-up` stay as they are, without any wait.

### Per machine

The tables, with every signal and its source:
`products/calc-u-59/reference/Idle.md` (TI-58, TI-58C, TI-59) and
`products/calc-u-1600/docs/developer/Idle.md` (PC-1500, PC-1500A, PC-1600).
What they changed against the first draft:

- **TI:** the scan loop holds with and without the printer (measured); the
  printer drain runs before the scan loop, so `runIdle`'s printer term never
  changes the answer.
- **PC-1500:** A adds the key read of INKEY$ (D9AA) to the key wait; B is
  the key wait with BUSY (764E bit 0) off and BREAKPARAM (788A) bits 5 and 6
  clear.
- **PC-1600:** the interrupt reads the key matrix every 1/64 s whatever
  runs, so "the matrix was read" is no signal. A becomes the ROM's own key
  wait (KEYWK2 bit 2) or key reads (KEYGETR, KEYDIRECT); a running program
  and a direct LOAD are then not A, and the direct-command rule goes. MODE 1
  changes nothing: the Z-80 runs BASIC in both modes.
- **OFF waits for A** on both Sharps: the ROM handles it in the editor only.

### What changes for users

- `wait`, presets and the driver: a direct LOAD/SAVE/COPY is waited for (as
  now); the plotter and printer are waited for too (new); the PC-1500 and
  the PC-1600 answer an INKEY$ program alike (idle).
- `key break` (and R/S, GTO, RST on the TI) go in at once.
- Calc-U-1600 at full speed: a waiting machine runs at real time, as
  Calc-U-59 does. The help's speed chapter says so.

## Decided

1. A program polling INKEY$, a PRINT pause, a CALLed program asking for a
   key: A and W, so settled, on every machine; not B.
2. A list of keys that do not wait, with holds, per machine.
3. Calc-U-1600 at full speed slows to real time while the machine waits,
   as Calc-U-59.
4. "Finished" (B) means the program has ended; the steps wait for settled.

## Steps

1. The tables, per machine, from what the research corpus says: a spec
   with the ROM regions, RAM bits, the emulator's signals (motor, pen,
   printer, DTR), the stretch and sample, and the keys that do not wait
   with their holds; what is not known says so. Calc-U-1600:
   `products/calc-u-1600/docs/developer/Idle.md`; Calc-U-59:
   `products/calc-u-59/reference/Idle.md`.
2. Tests of every case of the tables, against today's code: they show the
   differences listed here.
3. The library: `takesKeys`, W, settled and B; `IdleWatch` on W; pacing on
   W; the key list and `hold=` in the typing engine.
4. Calc-U-59: its table in code; `runIdle` goes.
5. Calc-U-1600: its table in code; the PC-1500's INKEY$ read; the PC-1600's direct
   command moves from the idle rule into A.
6. Glossary ("Idle" becomes "Finished" and "Settled", "Key readiness", a word for W), `docs/formats/preset.md`
   (idle), `docs/formats/harness.md` (`wait`, `key … hold=`), both products'
   `Harness.md`, the testing guide, the help's speed chapter.

Later, if the ROM versions make it worth it: the ROM part of each table
(ranges, RAM bits per variant) from a file per machine, the emulator's
signals (key matrix read, motor, DTR, printer busy) named by the core.

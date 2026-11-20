# Idle on the TI-58, TI-58C and TI-59

When the machine takes a key, waits, has settled, or has finished: the
signals, and how each question is answered from them. Addresses are those
of the commented listings `rom/TI59-commented.asm` (TI-59, TI-58) and
`rom/TI58C-commented.asm` (TI-58C); "measured" means with the harness's
`profile`.

| Question | Meaning | Asked by |
|---|---|---|
| A, takes keys | the ROM's keyboard scan looks for a key now | a scripted press before it goes down |
| W, waits | nothing happens until the user acts: A, or a card is awaited | pacing (full speed runs at real time while W holds, outside an operation) |
| settled | W held for 100 ms of emulated time, sampled every 1 ms | the steps: preset items, `key`, `type`, harness `wait` |
| B, finished | no program runs, and settled | `let-it-finish`, the wait "until the program has ended", the print listing |

## Signals

| Signal | Where | TI-59 | TI-58 | TI-58C |
|---|---|---|---|---|
| scan loop | PC range of the idle passes | 063C–0660 | 063C–0660 and the card veto 0459–045B | 063D–0659 and the printer rescan 0A25–0A2E |
| release gate | PC range after a key or a stop, while a key is still held (no auto-repeat) | 062A–063B | 062A–063B | 062A–063C |
| printer drain | 0627–0629: `PRT.STEP`, `TST.BUSY`, `JNC 0627`, before the release gate | yes | yes | yes |
| program runs | flag fB[0]; 0622 `TST fB[0]` branches to the run loop | 07C1 | 07C1 | 07BD |
| card wait | PC 16B2–16B7 (R/S at 16B2–16B4, card switch at 16B5–16B7) and the card switch seen by `TST BUSY` (`isWaitingForCard`) | yes | — | — |
| card transport | from 16B8 (`CLR.IDLE`, `CRD.RD`) to the end of the read or write | yes | — | — |
| Pause | PC 04EC–04F5, the display held (432 ms TI-59, 121 ms TI-58C) | yes | yes | yes |
| restore pending | the emulator's own: after a Reset of a TI-59 or TI-58 the RAM is put back at the first step into the scan loop | yes | yes | — (Reset is a power cycle) |

The scan loop holds with and without the PC-100C; it need not follow the
printer state. Measured idle passes:

| | without printer | with PC-100C |
|---|---|---|
| TI-59 | 063C–0642, 0656–0658 | 063C–0660 (the confirmation scan 0659–065F sees the printer line at D0 and drops it at 0660) |
| TI-58 | as the TI-59, and 0459–045B every pass | as the TI-59, and 0459–045B |
| TI-58C | 063D–0652 (0A25–0A2E never runs: only rows 10/11, the printer line at D10, lead there) | 063D–0659 and 0A25–0A2E every pass |

0A2F–0A30 (TI-58C) and 0661 (TI-59) run when a key is accepted: outside
the scan loop, so A drops while a key is taken, as it should.

## The questions

- **A** = no restore pending, and the PC in the scan loop.
- **W** = A, or the card wait (TI-59). Nothing else is busy while A holds:
  the printer drain runs before the release gate, so the printer has
  finished when the PC reaches the scan loop.
- **settled** = W for 100 ms, sampled every 1 ms (short beside the
  100 ms, so a break in the readiness of a running program is seen).
- **B** = A and fB[0] clear, settled. The scan loop is reached only through
  0622 with fB[0] clear, so A implies it; a card wait inside a program is
  W without B.

| Case | A | W | settled | B | Signal |
|---|---|---|---|---|---|
| idle, the display steady or flashing an error | yes | yes | yes | yes | scan loop (the error blink 0643–0655, TI-58C 063F–0649, is inside it) |
| a pending operand (STO, Fix, …) | yes | yes | yes | yes | scan loop |
| a key held after a stop | no | no | no | no | release gate |
| a program runs | no | no | no | no | fB[0], run loop |
| R/S pressed to start a program, still held | no | no | no | no | 07EC–07EE (TI-58C 07E0–07E2): it starts on the release |
| a Pause | no | no | no | no | 04EC–04F5 |
| Prt, TRACE, List, Adv on the PC-100C | no | no | no | no | print-wait loops, then the drain |
| Write waiting for a card, from the keyboard | no | yes | yes | yes | card wait (fB[0] clear) |
| Write waiting for a card, in a program | no | yes | yes | no | card wait, fB[0] set |
| Write, its key still held | no | no | no | no | 168A–168C waits for the release |
| a card being read or written | no | no | no | no | card transport |
| after Reset of a TI-59 or TI-58, before the first scan pass | no | no | no | no | restore pending |

## Keys that do not wait for A

These keys are read outside the scan loop, each by one `KEY` sample at a
fixed digit, with no arming and confirmation pair: the key must be down
when that instruction runs. A press of them by a command (the harness's or
the driver's `key`) waits for A at most 0.2 s, then goes down and stays
until the ROM has sampled it, at most a bound; a step may give its own
hold. A preset's key waits for A as any key does: its items run in order,
and there R/S continues a program once it has stopped.

| Key | Read | How often |
|---|---|---|
| R/S | between steps of a RAM program: TI-59 07DD–07DF, TI-58C 07D1–07D3 (stops the run); in the card wait 16B2 and the card run-out 16EA; in List 141F–1421 (TI-58C 1417–1419) | measured on `Lbl A GTO A`: 15 polls/s, about every 67 ms; not during a Pause, a long operation or a printer wait |
| GTO | between steps of a RAM program: TI-59 07CF, TI-58C 07CB; held, every second step lights the display for 256 passes (about 216 ms TI-59, 60 ms TI-58C) | as R/S; not when the memory is protected |
| RST | in a library program, at every module fetch: TI-59 0834–0836, TI-58C 0828–082A (stops the run, performs RST) | every module fetch |
| ADV, `.` | during Adv: TI-59 0F36–0F43, TI-58C 0F57–0F5B (feeds while held) | every feed step |

The polls between steps run only for a RAM program (Prg Src 0) and only at
a step boundary with no INV, operand or label byte pending (07CB–07CE);
a keycode sub-program, a library program and fast mode poll no R/S.

The hold: today's sample rule fits (at least 100 ms down, then until the
key's line has been sampled once more, at most 1 s): the bound must be
longer than a Pause (432 ms), during which no poll runs. GTO stepping is
the test's own hold.

## In the code

- A: `TIMachine::isKeyReady()` (`core/TIMachine.cpp`, the ranges in
  `core/TIRomRanges.hpp`).
- The rule: `app/TIPresetRules.cpp` (the card wait through the card
  reader's media place, B through fB[0] of `noteRun`); the `c59` print
  listing asks B.
- The keys that do not wait: `app/TITyping.cpp` (R/S, GTO, RST; at most
  0.2 s, then held until sampled).
- The cases as tests: `tests/idle_tests.cpp`.

## Not established

- How long a card read or write takes, and its whole path from 16B8.
- What `Write` does on the TI-58 (no card wait; whether it sets an error)
  and on the TI-58C.
- Whether the `KEY` at INV List 0F9D is its stop test.
- That a library program ignores R/S: from the listing only, not measured.
- The TRACE switch: a latch read at 069A, 05A9, 1549; not measured.
- The time from power-on to key readiness.

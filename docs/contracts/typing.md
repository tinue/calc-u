# Contract: typing strategy

Status: draft (2026-11-10). Forks T-1 to T-4 are decided (decision log);
open questions remain.

Inherits: C11 (key hold unit, `docs/inventory/divergence-matrix.md`),
ODD59-4, ADR 0009 (virtual time), Calc-U-1600's typing rules
(`docs/oddities-triage.md`: "2-frame hold, KBII sequences, Paste never
ENTER"). Builds on `machine.md` (M-1 ticks, M-3 key IDs) and
`frontend.md` (FE-4 one owner per key; Q-1 keys while busy).

Typing (glossary): feeding text into a machine as a sequence of key
presses, as when pasting text or from a preset. This contract also covers
single presses from host keys and on-screen keys, because they share the
hold rules.

## Today (facts)

| | Calc-U-59 (`KeySequencer`) | Calc-U-1600 (`MachineSession`) |
|---|---|---|
| Minimum hold | 100 ms, counted in 20 ms batches at their wall-time ends (`reference/SharedArchitecture.md:252-268`, `Shared/EmulatorSession.cpp:50-52`) | 2 emulated frames (1/60 s each), so it holds under pacing and turbo alike (`Decisions.md:585-595`) |
| Why | the ROM takes a key only when two scans one display sweep apart see it, and drops a shorter press as bounce (ODD59-4) | a quick tap can fall between two `advance()` calls and the scan never sees it; one frame is scanned on both models; a longer hold made fast typing overlap keys |
| Readiness | a tap waits for the steady scan loop before it presses and after it releases (scan-loop ranges per model in `MachineModelInfo`) | live keys: none. Scripted typing (preset `type:`, boot to prompt): waits until the ROM sits in the BASIC command loop for a stretch: PC-1500 PC in `E200-E4FF` (`Core/PC1500/PC1500TypedInput.hpp:51`); PC-1600 PC in `9280-9300` or a key-matrix read (port 37H) in the frame, for 15 frames (`Core/PC1600/PC1600BasicTyper.cpp:159-182`, `PC1600TypedInput.hpp:132`) |
| Live key queue | — (`KeySequencer` holds and waits, see above) | PC-1500 host keys except cursor keys: fixed cadence, 4 frames down and 4 frames gap in emulated cycles, 16 deep, a key typed into a full queue is dropped (`PC1500Machine::enqueueKey`, `advanceKeyQueue`, `PC1500Machine.cpp:349-377`) |
| Machine state read for typing | scan-loop position | PC-1600 SHIFT and KBII latches read from RAM `F64E`, `F3C6` before an accented character (`PC1600TypedInput.hpp:118-124`) |
| Two keys | one key at a time: a key-down while another is held releases the first | a key pressed during another key's deferred release waits for it plus the same gap; a double tap stays two taps |
| Sequences | two-code bindings (2nd + key) played as two full taps; their key-up is ignored | Shift sequences for shifted keys; PC-1600 accents as KBII, [SHIFT,] key, KBII; PC-1500 cursor keys as real presses (ROM auto-repeat) |
| Auto-repeat | OS auto-repeat ignored | — (cursor keys use the ROM's) |
| Paste | — | Paste Text types the first clipboard line and never presses ENTER; tool paths use `typeCommand()`, which does; any host key stops a paste |
| Focus loss | releases the held key | the frontend keeps which physical key to release |

## Responsibilities

- A **typing engine** in the library turns three kinds of input into key
  down and key up calls on the machine (M-3 key IDs): host keys from a
  frontend, on-screen key hits, and text (paste, preset typing, tool
  commands).
- It enforces the hold rules in emulated time (T-1), the readiness wait
  where the model has one (T-2) and the two-key rule (T-3).
- Its queue holds **presses, not taps**: a host key-down queues a press;
  if the key is already up when the press reaches the front, it is typed
  as a tap with the minimum hold; if the key is still held, it is pressed
  and stays down until the host key-up, so the ROM's own auto-repeat
  works as on the real machine. Host OS auto-repeat events are ignored.
- A product supplies a **typing profile** per model: host key and
  character to key-ID sequences (2nd, Shift, KBII, …), the minimum hold,
  and the fixed cadence for a model without key readiness.
- A machine may expose a **typing support** capability (T-2): key
  readiness, command-prompt readiness, and the state of input latches
  (SHIFT, KBII, …), each where the model has it.
- Paste Text never presses ENTER; tool paths may (adopted from
  Calc-U-1600). Any host key stops a running paste.
- Focus loss releases the held key (both products).

## Non-responsibilities

- Keyboard maps (host key → key ID) are product material; the engine
  applies them.
- What the frontend does with focus (FE-4).
- Preset step semantics (`wait:`, the speed steps, the implicit wait, ODD59-13): `docs/formats/preset.md`, contract 5.

## Invariants

- I-1: Every hold is measured in emulated time, so typing behaves the
  same at regular speed, at full speed and in virtual-time tests (M-1,
  ADR 0009).
- I-2: The engine never creates an overlap of two keys the typist didn't
  make (both products): queued presses and sequences start only when no
  key is down. Overlaps the typist makes pass through (T-3).
- I-3: Two runs fed the same input in the same emulated-time order type
  the same keys at the same ticks.

## Open forks

### T-1: Unit of the minimum hold (C11)

**Decided (2026-11-10): option (a), emulated time.**

Options:
- (a) **Recommended:** emulated time, with the minimum per model in the
  typing profile. TI-58/59: 100 ms of emulated time (today's number, now
  deterministic); PC-1500/1600: 2 frames = 1/30 s of emulated time. At
  full speed a hold passes in less wall time, which is what the ROM's
  debounce counts anyway (it counts scans, not seconds).
- (b) Wall time (Calc-U-59 today): a hold at full speed lasts many more
  scans than needed, and virtual-time tests and the harness cannot use
  it.

### T-2: Readiness wait (ODD59-4)

**Decided (2026-11-10): readiness for both products, through a typing
support capability** (revised option (a), after the user's questions):
- The machine exposes, where the model has it: **key readiness** (the
  ROM can take the next key now), **command-prompt readiness** (scripted
  typing may start a new line), and **latch states** (SHIFT, KBII, …).
- Key readiness gates only the start of the next queued press, except
  the first press of a preset step right after `skip-wait:`, which goes
  down at once (`docs/formats/preset.md` §5.1; 2026-11-13). A held
  key needs none: the ROM is in its key handling or auto-repeat, and the
  scan loop engages again once the key is released; the presses queued
  behind it wait anyway.
- TI-58/59: key readiness from the scan-loop ranges (today). PC-1600: key
  readiness from key-matrix reads, prompt readiness from the command-loop
  window, latches from RAM (today's reads, now behind the capability).
  PC-1500: prompt readiness from the command-loop window; key readiness
  is still to be identified, and its live keys keep the fixed 4 + 4 frame
  cadence until then (Q-3).

Correction to the first draft, which said "Calc-U-1600 does without": it
waits for the command loop in scripted typing, and the PC-1500 paces live
keys with a fixed cadence (facts table).

Facts: Calc-U-59 needs it (a tap that lands in the ROM's release gate or
debounce is lost). The app layer cannot read the ROM's position itself
(ADR 0006).

Options:
- (a) **Recommended:** the machine may expose a **key readiness**
  capability ("the ROM is in its key-scan loop now"); the engine waits
  for it before a tap and after a release when the profile says so. The
  TI-58/59 machines expose it from their scan-loop ranges; the Sharp
  machines don't need to.
- (b) Holds only, for every model: Calc-U-59 would lose keys typed fast
  or from presets (breaks ODD59-4's reason).

### T-3: Two keys at once

**Decided (2026-11-10): feed both** (the user's proposal). Keys the
typist physically holds together go to the key matrix as they are, with
no readiness wait, and the real ROM decides what happens, as on the
hardware. Line shorts and ghosting are not modelled today; they could be
refined inside a core later without touching this contract (machine
contract I-4). The engine itself never creates an overlap (I-2).
Consequence: Calc-U-59 changes slightly; a second key no longer releases
the first. The user's fallback, if this had not been workable: hold the
first key for the minimum, then feed the second.

Options:
- (a) **Recommended:** both rules, separated by what they handle. A key
  that arrives during another key's *deferred* release waits for it plus
  the same gap (Calc-U-1600, every model; keeps double taps). A key that
  arrives while another is still *physically held* follows the profile:
  TI-58/59 release the first (one key at a time, today); PC-1500/1600
  keep today's behaviour.
- (b) One rule for every model: one product's behaviour changes.

### T-4: Keys while an operation runs (frontend Q-1)

**Decided (2026-11-10): option (a), dropped.**

Facts: while busy, Calc-U-1600 runs operation slices instead of paced
emulation and refuses a second operation (`frontend.md` Q-1). What a
user key does then is not documented in either product.

Options:
- (a) **Recommended:** user keys arriving while busy are dropped; the
  operation (preset load, power cycle, typing a paste) owns the keyboard
  until it ends. A key that is still physically down when the operation
  ends is not pressed retroactively.
- (b) Queued and typed after the operation: surprising after a long
  preset run.

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Minimum hold (T-1) | 100 ms emulated | 2 frames (1/30 s) emulated | 2 frames emulated | unknown (Q-1) | unknown (Q-1) |
| Key readiness (T-2) | scan-loop ranges | to be identified (Q-3); fixed 4 + 4 frame cadence until then | key-matrix reads | unknown (Q-1) | unknown (Q-1) |
| Prompt readiness (T-2) | — | command loop `E200-E4FF` | command loop `9280-9300` | unknown (Q-1) | unknown (Q-1) |
| Latches (T-2) | — | — | SHIFT, KBII | unknown (Q-1) | unknown (Q-1) |
| Sequences | 2nd + key | Shift + key; cursor keys real presses | Shift + key; KBII accents | SHIFT, SML (`PC-140x-Keyboard.md`) | SHIFT (`PC-1350` overview) |
| Two keys held (T-3) | both fed | both fed | both fed | both fed | both fed |
| Paste Text | none today (no new feature) | yes, never ENTER | yes, never ENTER | — | — |

## Open questions

- Q-1 (deferred 2026-11-10: to an SC61860 product): Hold, readiness and sequences for the PC-1403 and PC-1350 come
  from their key-scan routines (SC61860 knowledge base,
  `PC-140x-Keyboard.md`); not needed before an SC61860 product exists.
- Q-3 (answered 2026-11-10, user, with `debugger.md` Q-1): when this is built
  in phase 6, the PC-1500's key-scan loop is researched and key readiness
  implemented; the fixed cadence then goes. Background: A key-readiness signal for the PC-1500 (for example key-matrix
  reads through the LH5811, as the PC-1600's port 37H reads); until it is
  found, the fixed cadence stays.
- Q-2 (phase 6, Calc-U-59 tests): Whether 100 ms of emulated time is enough everywhere the TI wall
  time worked, for example presses during a running program; to be
  checked by Calc-U-59's tests in phase 6.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | T-1 | Minimum holds are emulated time, per model in the typing profile: TI-58/59 100 ms, PC-1500/1600 2 frames. |
| 2026-11-10 | T-2 | Readiness for both products through an optional typing support capability (key readiness, prompt readiness, latch states). The queue holds presses, not taps; held keys stay down until key-up; host auto-repeat is ignored; readiness gates only the start of the next queued press. PC-1500 keeps its fixed cadence until a key-readiness signal is found. |
| 2026-11-10 | T-3 | Keys held together are fed to the matrix as they are; the ROM decides. The engine never creates an overlap. |
| 2026-11-10 | T-4 | User keys arriving while an operation runs are dropped. |
| 2026-11-10 | T-2 (amended in phase 4b, user) | Key readiness is part of the machine interface, not only of typing support: every machine reports whether it is ready to accept a key press. It is worded so, not as a "scan loop": the TI models have several such loops, depending on the model and on a printer. Typing support keeps prompt readiness and the latch states. |
| 2026-11-10 | T-2, T-4 (amended, user, phase 4b, batch 4 gate) | A live key goes down at once and is not held back for key readiness; readiness can be a short window, and a ROM samples some keys while a program runs (R/S and GTO on the TI models, BREAK on the Sharp ones), which must keep working as today. Typing outside a preset is not an operation: a host key stops a running paste. |
| 2026-11-13 | T-2 (amended, user, B-3) | The first press of a preset step right after `skip-wait:` goes down at once, ready or not (LIB-5 AC-14, LIB-6 AC-58). |

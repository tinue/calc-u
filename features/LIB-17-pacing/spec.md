# LIB-17: Pacing

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (ticks, the emulated-time type, advancing a machine).
- Contracts and decisions: `docs/contracts/frontend.md` (SP-1, I-6,
  "Rules that follow from decisions already taken"),
  `docs/contracts/machine.md` (M-1, F-1 to F-3); ADR 0009;
  `docs/oddities-triage.md` (ODD59-5, ODD59-6).

What the three status columns mean for this feature:
- **Library:** one pacer exists and is shown to keep time with a machine
  built for the tests and a wall clock supplied by the test.
- **Calc-U-59, Calc-U-1600:** the product paces its machines with the
  library's pacer and a pacing profile per model, its own pacer is gone,
  and the long-run test of that product gives the same result as before.

## User Stories
- As a user, I want a machine to run at the pace of the real device, so
  that a program, a `PAUSE`, a beep or a printout takes as long as on the
  real thing.
- As a user, I want that pace to hold over hours, so that the machine's
  clock and a long-running program do not fall behind or run ahead.
- As a user, I want to hold the display to run at full speed, in every
  product.
- As a user, I do not want the machine to race ahead after my computer
  was busy, a dialog was open or the product was in the background.
- As the developer, I want this written once: the same lag bug was fixed
  once in each product (`frontend.md`, SP-1).
- As a user watching the Calc-U-59 debug panel, I want the CPU view to
  keep moving while a ROM loop runs (ODD59-5).

## Out of Scope
- Who calls the pacer and when: the host's calls, pause and resume in
  the background, operations, and throwing away the sound of a
  full-speed stretch (owner: LIB-3).
- The gesture "hold the display", the `speed` step and the "elapsed"
  steps (owners: LIB-14 and C59-8 for the gesture; LIB-6 for the steps).
  This feature takes "regular speed" or "full speed" as an input.
- When a machine is ready to accept a key press (key readiness: LIB-1,
  C59-1) and when it needs a card (a media place that waits for a
  medium: LIB-7, C59-7). This feature takes "the machine waits for
  input" as an input from the machine session.
- Runs without a wall clock: the harness and tests in virtual time
  advance a machine directly and do not pace (owner: LIB-11).
- The tick rate and the weighting of instructions inside a core (LIB-1,
  C59-1, C1600-1); a machine's real-time clock (LIB-2).
- What the CPU tab of the Calc-U-59 debug panel shows (owner: C59-4).
- A speed multiplier (backlog; Calc-U-59 `TODO.md:6`).
- The known deviation of the PC-1600 (BASIC runs about 0.4 to 0.65 %
  fast, Calc-U-1600 `TODO.md:8`): it is in the core's timing, not in
  pacing, and stays as it is.

## Acceptance Criteria

### Library

Regular speed
- [ ] **AC-1** — Given a machine at regular speed and a caller that
  calls the pacer about 60 times a second, when any stretch of wall
  time has passed in which the lag never exceeded the limit of AC-4,
  then the machine's emulated time has grown by that stretch, within
  one slice of its pacing profile (for a profile whose slice is the
  call: within the time between two calls).
- [ ] **AC-2** — Given the calls come late, early or unevenly (timer
  jitter, oversleep), when a long stretch has passed, then the
  difference between emulated time and wall time is still within the
  bound of AC-1: errors of single calls do not add up (`frontend.md`
  I-6).
- [ ] **AC-3** — Given a machine that overshoots every slice by part of
  an instruction, when a long stretch has passed, then AC-1 and AC-2
  still hold: overshoots do not add up either.
- [ ] **AC-4** — Given the lag has grown beyond 0.1 seconds (the host
  was busy, a dialog was open), when the pacer is called, then the lag
  is dropped whole and not caught up, and pacing goes on from the moment
  of that call (Calc-U-59 today, `Shared/EmulationRunner.cpp:13-15, 120`;
  Calc-U-1600 runs 0.1 seconds of emulated time first today,
  `Shared/Pacer.cpp:32-40`).

Full speed
- [ ] **AC-5** — Given a machine at full speed, when the pacer is
  called, then it runs slice after slice without waiting, until the
  host time allowed for one call is used up, measured on the wall clock
  the caller supplies. The allowance is 14 ms of each call, one value of
  the library (Calc-U-1600 today, `Shared/Pacer.hpp:29`), so that the
  host has the rest of its roughly 16 ms to repaint and take input.
- [ ] **AC-6** — Given a machine is switched from full speed back to
  regular speed, then pacing goes on from that moment: the emulated time
  gained at full speed is neither waited out nor counted as lag.
- [ ] **AC-7** — Given a machine ran at full speed, when its tick count
  is read, then it is the emulated time the real device would have taken
  (`machine.md` F-3); pacing never changes a tick count.

Pacing profile
- [ ] **AC-8** — Given a model, then the product supplies a pacing
  profile for it with: the slice length, either fixed or "the call"
  (each call runs the emulated time due up to the wall time of that
  call); whether slice ends are jittered; and whether full speed is
  paced while the machine waits for input.
- [ ] **AC-9** — Given a profile with jitter, then each slice ends at an
  offset after its grid point that is drawn evenly from zero to half a
  slice; the offsets do not add up, and AC-1 to AC-3 hold as without
  jitter (Calc-U-59 `Shared/Pacing.hpp:25-45`).
- [ ] **AC-10** — Given a profile with jitter and a seed given by the
  caller, then the sequence of slice lengths is the same in every run
  with that seed; without a given seed it differs from run to run
  (Calc-U-59 `Shared/EmulationRunner.hpp:38`).
- [ ] **AC-11** — Given a profile with jitter and a machine that runs a
  loop whose length divides the slice length, when slices are run at
  regular speed, then the slices end at more than one position of the
  loop (the reason for ODD59-5, `Shared/Pacing.hpp:25-30`).
- [ ] **AC-12** — Given a profile whose full speed is paced while the
  machine waits for input, when the machine is at full speed and the
  machine session reports that condition for a slice, then nothing
  further is run until one slice length of wall time has passed since
  the last unpaced slice ended (ODD59-6; Calc-U-59
  `Shared/EmulationRunner.cpp:113-123`: a full-speed slice restarts the
  schedule).

Starting afresh
- [ ] **AC-13** — Given the machine session tells the pacer to start
  afresh (at the start, after a pause, after an operation), then pacing
  goes on from that moment with no lag and nothing to wait out.
- [ ] **AC-14** — Given the machine session hands the pacer a new
  machine with its pacing profile (a model or variant switch), then
  AC-13 applies and the new tick rate and profile are used from then on.

### Calc-U-59
- [ ] **AC-15** — Given the TI-59, TI-58 and TI-58C, then each has a
  pacing profile with a fixed slice of 20 ms, jitter (ODD59-5), and full
  speed paced while the machine waits for input (ODD59-6) (Calc-U-59
  `Shared/Pacing.hpp:9-33`; `frontend.md`, fit table).
- [ ] **AC-16** — Given a TI machine at full speed, when it waits for
  input, then it runs at the pace of regular speed; when it computes, it
  runs at full speed. It waits for input when it is ready to accept a
  key press, or, on the TI-59, when it needs a card now; a wait for the
  printer does not count. Today this is found from ROM address ranges
  that differ per model and with a printer present, and from retried
  polls of the keyboard and the card switch
  (`Shared/Pacing.cpp:26-29`, `Shared/Pacing.hpp:51-58`,
  `Shared/MachineModelInfo.hpp:59-105`).
- [ ] **AC-17** — Given the TI-58 showing a blinking error at regular
  speed, when the machine's program counter is read at 50 slice ends in
  a row, then it takes more than one value (`Shared/Pacing.hpp:25-30`).
- [ ] **AC-18** — Given Calc-U-59 before the pacer moves to the library
  and after, when the long-run test runs a TI-59, a TI-58 and a TI-58C
  at regular speed, then emulated time against wall time is the same in
  both, within the bound of AC-1 (`frontend.md`, SP-1: the user's
  condition; the two tests: Technical Requirements).
- [ ] **AC-19** — Given each frontend of Calc-U-59 (Swift iOS, Swift
  macOS, Qt6, Web), when a TI-59 runs at regular speed for one minute of
  wall time, then its emulated time has grown by one minute, within the
  bound of AC-1.

### Calc-U-1600
- [ ] **AC-20** — Given the PC-1500, PC-1500A and PC-1600, then each has
  a pacing profile whose slice is the call, with no jitter, and with
  full speed never paced (Calc-U-1600 `Shared/Pacer.cpp:21-61`).
- [ ] **AC-21** — Given Calc-U-1600 before the pacer moves to the
  library and after, when the long-run test runs a PC-1500A and a
  PC-1600 at regular speed, then emulated time against wall time is the
  same in both, within the bound of AC-1 (the two tests: Technical
  Requirements).
- [ ] **AC-22** — Given each frontend of Calc-U-1600 (Qt6, Web), when a
  PC-1600 runs at regular speed for one minute of wall time, then its
  emulated time has grown by one minute, within the bound of AC-1.

## Edge Cases
- **EC-1** — Given the same wall time is passed in two calls in a row,
  then the second call runs nothing and changes nothing.
- **EC-2** — Given full speed is switched on and off many times in a
  short while (the display tapped repeatedly), then AC-1 holds for the
  regular-speed stretches in between.
- **EC-3** — Given a machine that is paused by the debugger, then the
  pacer runs nothing, and AC-13 applies when it resumes.
- **EC-4** — Given a caller that calls 120 times a second, then AC-1 and
  AC-2 hold.
- **EC-5** — Given a caller that calls less often than every 0.1
  seconds (a throttled browser tab), then every call falls under AC-4
  and the machine runs slower than the real device; nothing fails.
- **EC-6** — Given a host too slow to run a machine at regular speed,
  then the lag grows to the limit of AC-4 and is dropped, again and
  again; the machine runs as fast as the host allows and nothing fails.

## Technical Requirements (optional)
- The pacer takes its wall clock from its caller, so that a test
  supplies its own; that clock never goes back (ADR 0009; both products
  test this way today, Calc-U-1600 `Shared/Pacer.hpp:21-27`).
- Emulated time is compared with an absolute wall-clock deadline, never
  with the length of the last slice (`frontend.md` I-6). This is the
  property both products fixed separately; it is stated here so that a
  later change cannot quietly undo it.
- No frontend and no product app layer holds a pacing rule of its own;
  what differs per model is in the pacing profile.
- The long-run test of AC-18 and AC-21 is two tests per product. One
  uses a wall clock supplied by the test, with jitter, oversleep and
  stalls scripted and hours of wall time simulated in seconds, and
  asserts the bound of AC-1 exactly; it belongs to the frequent group.
  One runs a real build on the real clock for ten minutes and asserts
  that emulated time and wall time differ by no more than the bound of
  AC-1 at the end; it belongs to the checkpoint group. Both exist before
  the product's pacer is replaced and are run on the old code first.
- How AC-19 and AC-22 read a running frontend's emulated time where
  harness attach does not exist yet (Qt6, Web;
  `docs/contracts/harness.md`, H-4) is decided in the design.

## Open Questions
None.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| One pacer in the library, a pacing profile per model (slice length, jitter, full speed paced while the machine waits) | `frontend.md` SP-1 | 2026-11-10 |
| The algorithm is carried over unchanged in effect; the long-run rate stays exact, checked per product before and after | `frontend.md` SP-1 (the user's condition), I-6 | 2026-11-10 |
| Lag beyond about 0.1 s is dropped | `frontend.md`, rules from decisions already taken (both products today) | 2026-11-10 |
| ODD59-5 (jitter) and ODD59-6 (full speed paced in the scan loop) stay, as pacing parameters of the TI machines | `docs/oddities-triage.md` | 2026-11-09 |
| Holding the display runs at full speed in every product | `frontend.md`, rules from decisions already taken | 2026-11-10 |
| A machine counts its own emulated time; pacing only compares it with the wall clock | `machine.md` M-1, F-1 to F-3 | 2026-11-10 |
| The TI slice stays 20 ms; the Sharp slice is the call, as today | `frontend.md`, fit table; Calc-U-1600 `Shared/Pacer.cpp:21-43` | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-11): the numbers of AC-17 (50 slice ends) and AC-19, AC-22 (one minute) | They make the criteria checkable; no source gives them | 2026-11-10 |
| The long-run test is two tests per product: a scripted clock (frequent group) and ten minutes on the real clock (checkpoint group) | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| "Slice" and "lag" are glossary terms | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| Full speed is paced while a TI machine is ready to accept a key press or needs a card now; the wording is not "key scan loop", because the TI models have several such loops, depending on the model and on a printer | user, 2026-11-10, batch 2 gate; amends `frontend.md` SP-1 | 2026-11-10 |
| The new pacer carries the overshoot of every advance (AC-3). Calc-U-1600 drops it today, so the before-and-after test of AC-21 states the measured old drift as the expected difference | user, 2026-11-10, batch 2 gate | 2026-11-10 |
| A lag over 0.1 seconds is dropped whole, for every model | user, 2026-11-10, batch 2 gate (prompted) | 2026-11-10 |
| The full-speed allowance is one library value, 14 ms per call. The Calc-U-59 Swift and Qt6 frontends lose the free-running thread of today and with it a little top speed | Recommendation applied; the user had no preference and asked what the problem is, which was explained at the gate | 2026-11-10 |

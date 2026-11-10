# LIB-4: Control state

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-2 (places, what fits them, whether an attach would
  succeed and why not), LIB-3 (state topics, busy, product entries, host
  services).
- Contracts and decisions: `docs/contracts/frontend.md` (FE-2, FE-6, fit
  table), `docs/contracts/machine.md` (M-4, M-8); ADR 0024, 0025;
  `docs/behaviour-changes.md` rows 59-9, 59-14.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`.

What the three status columns mean for this feature:
- **Library:** the control state is computed from the descriptors and
  the running machine and sent as a state topic, shown with machines
  built for the tests.
- **Calc-U-59, Calc-U-1600:** every frontend of the product takes what
  it offers around the machine from the control state; no frontend and
  no product code decides it from the model. A column is complete
  together with the rows that draw it (LIB-14, C59-8).

## User Stories
- As a user, I want the controls around the machine to show exactly what
  this model and what is plugged into it offer, and to see why something
  cannot be chosen right now.
- As a user of two Calc-U products, I want the same kind of control to
  behave the same in both.
- As the developer of a frontend, I want to draw what I am told, so that
  a rule exists once and three frontends cannot disagree.
- As the developer of a product, I want to add what only my product
  offers without changing the library.
- As an agent, I want to read through the harness what a machine offers
  right now.

## Out of Scope
- Drawing: the control bar, the menus, the pickers and the panels
  (owners: LIB-14, LIB-7, LIB-8, C59-8). Where a control sits (bar or
  menu) and the layout per device are theirs.
- The commands that act on a choice: model and variant switch, attach
  and detach (owner: LIB-18).
- The state of each medium: what is in a media place, its name, whether
  it can be saved, the lists of templates and instances (owner: LIB-7).
  This feature lists the media places.
- The busy topic itself (owner: LIB-3).
- Run state and the debugger's controls (owner: LIB-10); the content of
  the debug panel (C59-4).
- ROMs that are missing (owner: LIB-12); this feature shows the result
  as "not available now" with the reason LIB-12 gives.
- Settings and what the settings dialog offers (LIB-9, LIB-14).

## Acceptance Criteria

### Library
- [ ] **AC-1** — Given an app session with a running machine, then the
  control state is a state topic (LIB-3) that holds:
  the product's models, and which one runs;
  the variants of the running model, and which one runs;
  each place of the running machine and of the peripherals plugged into
  it, with its name, what is plugged in, and the peripherals that fit
  it, each with its variants, whether it is available now and, if not,
  the reason;
  each media place of the machine and of the peripherals plugged in,
  with its kind, whether it is pass-through or holding, and its sides;
  the panels the machine offers now: one paper panel per paper output
  capability;
  and the product's own entries (`frontend.md` FE-2, FE-6).
- [ ] **AC-2** — Given a peripheral that fits a place, then "available
  now" and the reason are what the machine answers when asked whether it
  can be attached there now (LIB-2 AC-6).
- [ ] **AC-3** — Given an operation runs, then every entry whose choice
  would start an operation (a model, a variant, a peripheral for a
  place) is not available now, with the reason that an operation runs;
  when it ends, the entries are as before.
- [ ] **AC-4** — Given the machine is replaced, a peripheral is attached
  or detached, or an operation ends, then the control state is computed
  again and sent if it differs (LIB-3 AC-5).
- [ ] **AC-5** — Given a model that lacks a place, a media place or a
  panel, then the control state has no entry for it; nothing says "not
  on this model" (ADR 0024: "items a product lacks simply don't
  appear").
- [ ] **AC-6** — Given a product's app layer, then it can add entries of
  its own to the control state; they are computed below the frontend
  like the generic ones, the library carries them without reading them,
  and only that product's frontends read them (`frontend.md` FE-6).
- [ ] **AC-7** — Given two frontends of a product on the same machine in
  the same state, with the same host services, then both are sent the
  same control state.
- [ ] **AC-8** — Given the harness, then the control state can be read
  with the command that reads a topic (LIB-11).

### Calc-U-59
- [ ] **AC-9** — Given Calc-U-59, then the control state lists the
  models TI-59, TI-58 and TI-58C and no variants
  (`Qt6/app/ControlBar.cpp:46-50`, `Web/src/ControlBar.jsx:90-100`).
- [ ] **AC-10** — Given any TI model, then the control state lists the
  place `slot-1` with the 14 Solid State ROM modules and the empty slot,
  and the place `printer` with the PC-100C. Today the module is chosen
  in the settings and the printer by a toggle in the printer panel
  (`Shared/SessionController.cpp:220-241, 362-371`; behaviour change
  59-9).
- [ ] **AC-11** — Given the TI-59, then the control state lists the card
  reader as a media place; given the TI-58 or the TI-58C, then it lists
  none, and no frontend shows a control for cards. Today each frontend
  hides the card buttons by its own rule, and the Qt6 File menu shows
  "Fit Card…" on every model
  (`App/Views/CalculatorView.swift:347-367`, `App/MachineModel.swift:16-33`,
  `Qt6/app/ControlBar.cpp:93-100`, `Qt6/app/MainWindow.cpp:57-58`,
  `Web/src/ControlBar.jsx:85, 103-111`; behaviour change 59-14).
- [ ] **AC-12** — Given a PC-100C is plugged in, then the control state
  lists a paper panel; given none, then it lists no paper panel.
- [ ] **AC-13** — Given Calc-U-59, then its product entries say whether
  the debug panel is offered and which of its parts can be used now, as
  the three frontends work it out today from the same state: FREEZE and
  FREEZE ON START, RESUME and STEP only in the panel that owns the
  freeze, the CALCULATOR tab while an ASM overlay runs, Run for an
  overlay with words, the trace switch
  (`App/Views/LiveDebugView.swift:45-87`,
  `Qt6/app/debug/DebugPanel.cpp:56-61`, `Web/src/Debugger.jsx:43-58, 316`;
  the content: C59-4).
- [ ] **AC-14** — Given the Swift, Qt6 and Web frontends of Calc-U-59,
  then none keeps a copy of a model fact; the copy the Swift frontend
  has today (`App/MachineModel.swift:16-33`) is gone.

### Calc-U-1600
- [ ] **AC-15** — Given Calc-U-1600, then the control state lists the
  models PC-1500, PC-1500A and PC-1600, and the variants of the running
  model: A01, A03, A04 on the PC-1500; New, Old on the PC-1600; none to
  choose on the PC-1500A (`Shared/ControlState.cpp:38-40`).
- [ ] **AC-16** — Given what today's control state holds
  (`Shared/ControlState.hpp:27-56`, `Shared/ControlState.cpp:7-48`), then
  each field arrives as follows:

  | Today | Arrives as |
  |---|---|
  | `model` | models (AC-1) |
  | `slotCount` | the places `slot-1` and, on the PC-1600, `slot-2` |
  | `ce150`, `ce1600p`: `offered`, `attached`, `enabled` | the place `printer`: what fits, what is plugged in, available now with the reason (LIB-2 AC-29) |
  | `ce158`: `offered`, `attached`, `enabled` | the place `interface`, likewise |
  | `ce150/ce158/ce1600p.onControlBar` | not in the control state: where a control sits is the frontend's (LIB-14) |
  | `pc1500RomMenu`, `pc1600RomMenu` | the variants of the model |
  | `ce1600pRomMenu` | the variants of the CE-1600P in the place `printer` |
  | `floppyEnabled`, `tapeShown` | the media places of the CE-1600F and of the cassette, listed while their peripheral is plugged in |
  | `floppyShown` | a product entry: the disk controls are shown on every PC-1600 and usable only while the CE-1600P is plugged in |
  | `hostDriveOffered`, `hostDriveDir` | the place `host-drive`, available only when the frontend has a folder picker (LIB-3 AC-26); the folder shown is a product entry |
  | `paper` | one paper panel for the plugged-in printer (AC-1) |
  | `printerShown` | a product entry: the CE-158's printer panel while a CE-158 is plugged in |

- [ ] **AC-17** — Given the Qt6 and the Web frontend, then both are sent
  the same control state (AC-7). Today the Web frontend is not sent the
  host drive's fields, and the Qt6 frontend ignores `model` and the
  `offered` of the CE-150 and CE-158 (`Web/bindings.cpp:709-767`,
  `Qt6/app/MainWindow.cpp:333-341, 552-554, 1001-1002`).
- [ ] **AC-18** — Given an operation runs in Calc-U-1600, then AC-3
  holds in both frontends. Today the Qt6 frontend disables nothing and
  swallows input, and the Web frontend disables the menus and the model
  and slot pickers but not the peripheral buttons, the disk picker or
  the tape controls (`Qt6/app/SyncOperations.cpp:47-78`,
  `Web/src/CalcU1600.jsx:259-294`, `Web/src/ControlBar.jsx:77, 91`).

## Edge Cases
- **EC-1** — Given a place that holds a peripheral, then the entry of
  that peripheral says it is plugged in, and choosing it again is no
  change (today both products ignore a pick of what is already chosen,
  Calc-U-1600 `Qt6/app/MainWindow.cpp:1023-1045`, Calc-U-59
  `Qt6/app/ControlBar.cpp:46-50`).
- **EC-2** — Given a peripheral whose ROM file is missing, then its
  entry is not available now, with the reason LIB-12 gives.
- **EC-3** — Given a frontend without a folder picker (the Web
  frontend), then the place `host-drive` is not available, with that
  reason.
- **EC-4** — Given a module definition is added to the save folder while
  the product runs, then the entries of the places it fits include it
  after the next time the control state is computed (the lists of
  templates and instances: LIB-7).

## Technical Requirements (optional)
- No rule about what a machine offers lives in a frontend, in the
  bridge or in the web bindings (`frontend.md` FE-2, I-3). The rules a
  frontend keeps are about itself: its layout, its focus, whether its
  clipboard holds text.
- Nothing in the generic part of the control state names a model, a
  peripheral type or a product (`frontend.md` I-4): it is described with
  the descriptors' terms.

## Open Questions
- [ ] **OQ-1** — AC-3 puts "an operation runs" into the control state as
  a reason, so that no frontend needs a rule of its own for it. The
  contract only says that such commands are refused while busy
  (`frontend.md` Q-1). The other way is that every frontend disables
  those controls itself from the busy topic. *Recommendation:* as in
  AC-3; it is the one place where the two Calc-U-1600 frontends disagree
  today.
- [ ] **OQ-2** — The disk controls of the PC-1600 are shown also without
  a CE-1600P, "so the bar doesn't jump" (`frontend.md` FE-6, facts).
  AC-16 keeps that as a product entry. Should a media place that could
  exist be a generic notion instead, so that any product can show a
  control before its peripheral is plugged in? *Recommendation:* keep it
  a product entry until a second product needs it (`capabilities.md`
  C-1).
- [ ] **OQ-3** — Does the control state carry the name and state of what
  is in each media place, or only the place (AC-1)? The Web frontend of
  Calc-U-1600 sends both in one message today
  (`Web/bindings.cpp:709-767`). *Recommendation:* only the place; the
  medium's state is a topic of LIB-7, so that a card passing through
  the reader does not resend the whole control state.
- [ ] **OQ-4** — The debug panel entries of Calc-U-59 (AC-13): are they
  product entries of the control state, or part of the run state topic
  of LIB-10, whose pause, step and resume controls are generic?
  *Recommendation:* the generic three follow LIB-10; what only the
  Calc-U-59 panel has stays a product entry, specified in C59-4.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The rules about which controls a machine offers live in the library, as one generic control state computed from the descriptor and the live machine | `frontend.md` FE-2 | 2026-11-10 |
| A product's app layer may add control entries of its own, computed below the frontend | `frontend.md` FE-6 | 2026-11-10 |
| Presets and the control state name places, not connectors | `machine.md` M-8 | 2026-11-10 |
| One menu and control-bar structure; items a product lacks do not appear | ADR 0024 | 2026-11-09 |
| A module is chosen with a picker for its slot | ADR 0025; behaviour change 59-9 | 2026-11-09 |
| Proposed in this spec, for the gate: while an operation runs, the entries that would start one are not available, with that reason (AC-3) | No contract says where this rule lives; the Calc-U-1600 frontends differ today | 2026-11-10 |

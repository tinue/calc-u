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
- Contracts and decisions: `docs/contracts/frontend.md` (FE-2, FE-6, Q-1,
  fit table), `docs/contracts/machine.md` (M-4, M-8),
  `docs/contracts/presets.md` (PR-9); ADR 0024, 0025;
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
  and detach, and what choosing the current entry again does (owner:
  LIB-18).
- The state of each medium: what is in a media place, its name, whether
  it can be saved, the lists of templates and instances, and how a slot
  picker joins them with the modules that fit (owner: LIB-7; OQ-3).
- The busy topic itself (owner: LIB-3).
- Run state and the debugger's controls (owner: LIB-10); what the
  Calc-U-59 debug panel offers and when (owner: C59-4).
- Settings and what the settings dialog offers (LIB-9, LIB-14).

## Acceptance Criteria

### Library
- [ ] **AC-1** — Given an app session, then the control state is a state
  topic (LIB-3) that holds, each entry under the name that commands and
  presets use for it and in the order of the descriptors:
  the product's models, and which one runs;
  the variants of the running model, and which one runs;
  each place of the running machine and of the peripherals plugged into
  it, with what is plugged in and in which variant, the empty place as a
  choice where the place can be empty, and the peripherals that fit it
  by LIB-2 AC-2, each with its variants;
  each media place of the machine and of the peripherals plugged in,
  with its owner, its kind, whether it is pass-through or holding, and
  its sides;
  the panels: one paper panel in a product that has printers, always,
  with the printer that is plugged in as its owner, or none (LIB-8
  AC-12);
  the actions that start an operation and are not a choice among
  entries: Reset, Cold boot, and loading a preset or opening a file,
  each available now or not, with the reason;
  and the product's own entries (`frontend.md` FE-2, FE-6).
- [ ] **AC-2** — Given a model, a variant, or a peripheral or variant
  for a place, then its entry says whether it is available now, and if
  not, gives one reason: the first that applies of
  an operation runs (AC-3);
  the product's app layer says so, with its reason (AC-7);
  the machine answers no when asked whether it can be attached there
  now, with the machine's reason (LIB-2 AC-6).
- [ ] **AC-3** — Given an operation runs, then every entry whose choice
  would start an operation is not available now, with the reason that an
  operation runs, except the models and the actions Reset and Cold boot,
  which end the running operation (LIB-18 AC-16); when the operation has ended, the control state is
  computed again from the machine as it now is.
- [ ] **AC-4** — Given the end of any call of the host and of any
  command, then the control state is computed, and sent if it differs
  from what was last sent (LIB-3 AC-5).
- [ ] **AC-5** — Given a model that lacks a place, a media place or a
  panel, then the control state has no entry for it; nothing says "not
  on this model" (ADR 0024: "items a product lacks simply don't
  appear").
- [ ] **AC-6** — Given the product could not start because a ROM file
  is missing (LIB-12 AC-5), then no control state is sent.
- [ ] **AC-7** — Given a product's app layer, then it can add entries of
  its own to the control state, and can make a generic entry not
  available with a reason of its own; both are computed below the
  frontend, the library carries the entries without reading them, and
  only that product's frontends read them (`frontend.md` FE-6).
- [ ] **AC-8** — Given two frontends of a product on the same machine in
  the same state, then both are sent the same control state, apart from
  what a product entry makes depend on the frontend (AC-18).
- [ ] **AC-9** — Given the control state, then it can be read like any
  other state topic, also by the harness.

### Calc-U-59
- [ ] **AC-10** — Given Calc-U-59, then the control state lists the
  models TI-59, TI-58 and TI-58C and no variants
  (`Qt6/app/ControlBar.cpp:46-50`, `Web/src/ControlBar.jsx:90-100`).
- [ ] **AC-11** — Given any TI model, then the control state lists the
  place `slot-1` with the 14 Solid State ROM modules of C59-6 and the
  empty slot, and the place `printer` with the PC-100C and the empty
  place. Today the module is chosen in the settings, an empty slot
  cannot be chosen, and the printer is connected by a toggle in the
  printer panel (`Shared/SessionController.cpp:220-241, 362-371`;
  `presets.md` PR-9; behaviour change 59-9; the empty slot: OQ-5).
- [ ] **AC-12** — Given the TI-59, then the control state lists the card
  reader as a media place; given the TI-58 or the TI-58C, then it lists
  none, and no frontend shows a control for cards. Today each frontend
  hides the card buttons by its own rule, and the Qt6 File menu shows
  "Fit Card…" on every model
  (`App/Views/CalculatorView.swift:347-367`, `App/MachineModel.swift:16-33`,
  `Qt6/app/ControlBar.cpp:93-100`, `Qt6/app/MainWindow.cpp:57-58`,
  `Web/src/ControlBar.jsx:85, 103-111`; behaviour change 59-14).
- [ ] **AC-13** — Given Calc-U-59, then the control state always lists
  the paper panel, owned by the PC-100C while it is plugged in and by
  nothing otherwise (LIB-8 AC-12; user, batch 6 gate).
- [ ] **AC-14** — Given Calc-U-59, then its product entries can say what
  its debug panel offers now (specified in C59-4; OQ-4).
- [ ] **AC-15** — Given the Swift, Qt6 and Web frontends of Calc-U-59,
  then all three draw the model, the module, the printer and the card
  controls from the control state; the copy of the model facts the Swift
  frontend has today (`App/MachineModel.swift:16-33`) is gone.

### Calc-U-1600
- [ ] **AC-16** — Given Calc-U-1600, then the control state lists the
  models PC-1500, PC-1500A and PC-1600, and the variants of the running
  model: A01, A03, A04 on the PC-1500; New, Old on the PC-1600; the one
  variant of the PC-1500A (`Shared/ControlState.cpp:38-40`).
- [ ] **AC-17** — Given what today's control state holds
  (`Shared/ControlState.hpp:27-56`, `Shared/ControlState.cpp:7-48`), then
  each field arrives as follows:

  | Today | Arrives as |
  |---|---|
  | `model` | models (AC-1) |
  | `slotCount` | the places `slot-1` and, on the PC-1600, `slot-2` |
  | `ce150`, `ce1600p`: `offered`, `attached`, `enabled` | the place `printer`: what fits, what is plugged in, available now with the reason (LIB-2 AC-29) |
  | `ce158`: `offered`, `attached`, `enabled` | the place `interface`, likewise |
  | `ce150/ce158/ce1600p.onControlBar` | goes: every place that is not a module slot has a control on the bar by the family rule of LIB-14 AC-9 (user, 2026-11-10, batch 5 gate) |
  | `pc1500RomMenu`, `pc1600RomMenu` | the variants of the model |
  | `ce1600pRomMenu` | the variants of the CE-1600P in the place `printer`, and which one runs |
  | `floppyEnabled`, `tapeShown` | the media places of the CE-1600F and of the cassette, listed while the peripheral that owns them is plugged in |
  | `floppyShown` | a product entry: the disk controls are shown on every PC-1600 and usable only while the CE-1600P is plugged in |
  | `hostDriveOffered`, `hostDriveDir` | the place `host-drive` (AC-18); the folder shown is a product entry |
  | `paper` | one paper panel owned by the plugged-in printer |
  | `printerShown` | a product entry: the CE-158's printer panel while a CE-158 is plugged in |

- [ ] **AC-18** — Given a frontend that says it cannot mount a host
  folder (the Web frontend), then Calc-U-1600's app layer makes the
  place `host-drive` not available, with that reason; it is the only
  entry in which the control state of the Qt6 and the Web frontend
  differ for the same machine (today `hostDirectories`,
  `Shared/AppSession.hpp:82-84`, `Web/bindings.cpp:656`).
- [ ] **AC-19** — Given the Qt6 and the Web frontend, then both draw
  from the whole control state. Today the Web frontend is not sent the
  host drive's fields, and the Qt6 frontend ignores `model` and the
  `offered` of the CE-150 and CE-158 (`Web/bindings.cpp:709-767`,
  `Qt6/app/MainWindow.cpp:333-341, 552-554, 1001-1002`).
- [ ] **AC-20** — Given an operation runs in Calc-U-1600, then AC-3
  holds for the model, the variants and the places in both frontends.
  Today the Qt6 frontend disables nothing and swallows input, and the
  Web frontend disables the menus and the model and slot pickers but not
  the peripheral buttons, the disk picker or the tape controls
  (`Qt6/app/SyncOperations.cpp:47-78`, `Web/src/CalcU1600.jsx:259-294`,
  `Web/src/ControlBar.jsx:77, 91`).

## Edge Cases
- **EC-1** — Given a place that holds a peripheral, then the entry of
  that peripheral says it is plugged in.
- **EC-3** — Given a module definition is read after the catalogue was
  built (LIB-2 AC-2), then the places it fits list it the next time the
  control state is computed (AC-4).
- **EC-4** — Given a peripheral that brings another with it over a
  connector that is not modelled (the CE-1600F with the CE-1600P), then
  the control state lists no place for the second; its media place is
  listed with the first as owner.
- **EC-5** — Given several reasons apply to one entry, then the one that
  comes first in AC-2 is given.

## Technical Requirements (optional)
- No rule about what a machine offers lives in a frontend, in the
  bridge or in the web bindings (`frontend.md` FE-2, I-3). The rules a
  frontend keeps are about itself: its layout, its focus, whether its
  clipboard holds text.
- Nothing in the generic part of the control state names a model, a
  peripheral type or a product (`frontend.md` I-4): it is described with
  the descriptors' terms.

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* How far the control state reaches while an operation
  runs. `frontend.md` Q-1 refuses every command that would start an
  operation: also Reset, Cold boot, loading a preset and inserting a
  medium, which are menu items and media controls, not entries of the
  control state. Either the control state also lists those actions, each
  available now or not with a reason, so that every frontend draws them
  from one rule, the Swift one included; or it covers only its own
  entries (AC-3) and the other controls follow the busy topic, by a rule
  the frontends share. *Recommendation:* list the actions too; it is the
  point on which the two Calc-U-1600 frontends disagree today. This
  widens what FE-2 lists as the control state's content.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* Where a peripheral's control sits. Today a rule by
  model decides whether the CE-150, CE-158 and CE-1600P have a control
  on the bar or only in the menu (`Shared/ControlState.cpp:22-30`). ADR
  0024 puts on the bar "the model first, then one picker per module slot
  and per media place" and says nothing about the places `printer` and
  `interface`. AC-17 keeps today's rule as a product entry.
  *Recommendation:* decide a family rule in the LIB-14 spec; the product
  entry then goes.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* What a module slot's entry lists, and what the control
  state says about media. A Sharp slot picker shows module kinds,
  bundled templates and the user's saved instances together today
  (`Web/bindings.cpp:736-748`). AC-1 lists the peripherals that fit, one
  per descriptor or module definition; the names and states of media are
  LIB-7's topic. *Recommendation:* as in AC-1; the picker joins the two
  topics, and how is settled in the LIB-7 spec. A card passing through
  the TI-59's reader then does not resend the control state.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The debug panel of Calc-U-59 (AC-14): are the
  conditions of its controls product entries of the control state, or
  part of the run state topic of LIB-10, whose pause, step and resume
  controls are generic? *Recommendation:* the generic three follow
  LIB-10; what only the Calc-U-59 panel has is a product entry,
  specified in C59-4.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The empty module slot in the Calc-U-59 picker (AC-11).
  `presets.md` PR-9 allows a TI machine without a module and notes that
  no frontend offers it today; row 59-9 does not mention it.
  *Recommendation:* the picker offers it, and row 59-9 says so.
- [x] **OQ-6** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The reason of AC-18. Today the test is whether the
  frontend "can mount a host directory"; LIB-3 AC-26 lists a folder
  picker among the host services, which a browser may have without being
  able to give the session a folder to work in. *Recommendation:* keep
  today's meaning, as a statement the Calc-U-1600 frontends make to
  their own app layer, not as a generic host service.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The rules about which controls a machine offers live in the library, as one generic control state computed from the descriptor and the live machine | `frontend.md` FE-2 | 2026-11-10 |
| A product's app layer may add control entries of its own, computed below the frontend | `frontend.md` FE-6 | 2026-11-10 |
| Presets and the control state name places, not connectors | `machine.md` M-8 | 2026-11-10 |
| One menu and control-bar structure; items a product lacks do not appear | ADR 0024 | 2026-11-09 |
| A module is chosen with a picker for its slot | ADR 0025; behaviour change 59-9 | 2026-11-09 |
| Proposed in this spec, for the gate: every model, variant and peripheral entry carries "available now" with one reason, in the order of AC-2; a product's app layer can make a generic entry not available (AC-7); while an operation runs, the entries that would start one are not available (AC-3) | FE-6 names "available now and a reason" for places only; no contract says where the rule for a running operation lives, and the Calc-U-1600 frontends differ today | 2026-11-10 |
| The control state also lists the actions that start an operation (Reset, Cold boot, loading a preset or opening a file), so that every frontend disables them by one rule | user, 2026-11-10, batch 3 gate; widens `frontend.md` FE-2 | 2026-11-10 |
| The Calc-U-59 module picker offers the empty slot | user, 2026-11-10, batch 3 gate; ledger row 59-9 extended | 2026-11-10 |
| With the full ROM set required at the start (LIB-12), no entry is ever unavailable for a missing ROM | user, 2026-11-10, batch 3 gate | 2026-11-10 |

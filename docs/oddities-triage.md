# Deliberate oddities: triage

Every deliberate oddity found in phase 1 either becomes an ADR or stays in
its product's decisions file (`docs/Process.md`, phase 3). Sources: the
`ODD59-n` table in [`inventory/calc-u-59.md`](inventory/calc-u-59.md) and
the grouped list of Calc-U-1600's `docs/background/Decisions.md` in
[`inventory/calc-u-1600.md`](inventory/calc-u-1600.md). No oddity needed an
ADR of its own; the cross-product ones are covered by the phase 3 ADRs.

Outcomes:
- **Stays**: kept in the product's decisions file, carried over in the
  migration as it is.
- **Superseded by ADR n**: the phase 3 decision replaces it; the decisions
  file entry is removed or rewritten with the code (phase 6).
- **Adopted by ADR n**: the product's rule became the family rule.
- **Phase 4**: an input to the named contract, which settles it.

## Calc-U-59

| ID | Oddity (short) | Outcome |
|---|---|---|
| ODD59-1 | 26 of 45 keys on the host keyboard | Stays (Briefing §1.3: key tables stay) |
| ODD59-2 | Shift is a 2nd prefix only for letters | Stays |
| ODD59-3 | Debug and printer panels take no calculator keys | Phase 4: frontend components (one input surface) |
| ODD59-4 | Key held ≥ 100 ms wall time; tap waits for the scan loop | Phase 4: typing strategy (C11; virtual time, ADR 0009) |
| ODD59-5 | Random jitter on batch ends | Stays (pacing parameter of the TI machines) |
| ODD59-6 | Full speed still sleeps in the scan loop | Stays |
| ODD59-7 | Preset parse errors reported, load continues | Superseded by ADR 0017 |
| ODD59-8 | A preset keeps the current module and printer | Superseded by ADR 0016 (v1 reader fills them in) |
| ODD59-9 | `#` starts a comment anywhere | Stays for the v1 reader; phase 4: preset contract |
| ODD59-10 | Registers written after the program, as given | Stays; phase 4: preset contract |
| ODD59-11 | A damaged card file is refused whole | Stays |
| ODD59-12 | Version 1 cards stay version 1 when written | Superseded by ADR 0020 |
| ODD59-13 | Adv in KEYSTROKES needs `RegularSpeed:` | Stays for the v1 reader; phase 4: preset contract |
| ODD59-14 | A load runs 300,000 power-on steps before writing RAM | Stays (part of the TI cold boot) |
| ODD59-15 | TI-58C extra registers `H00`–`H03` | Stays |
| ODD59-16 | Leftmost display position: brighter of C and glyph | Stays |
| ODD59-17 | C indicator in RUN counts set `fA` bits | Stays |
| ODD59-18 | Zero suppression darkens ctrl 2 and 4, 5 → minus | Stays |
| ODD59-19 | Printer line BUSY 4165, not 4168 | Stays |
| ODD59-20 | `WAIT.BUSY` is a no-op | Stays |
| ODD59-21 | TRACE latch survives reset | Stays |
| ODD59-22 | TI-58 card-veto poll counted as scan loop | Stays |
| ODD59-23 | Web: LED Classic only; cards last for the page session | Split: LED Classic stays; card persistence superseded by ADR 0019 (media in the save folder, persistent on the web too) |
| ODD59-24 | Qt macOS build never shipped | Stays (release scope) |
| ODD59-25 | Qt keeps running in the background; Swift and web pause | Stays (same split as Calc-U-1600, M17) |
| ODD59-26 | iPhone: debug page off by default | Stays |
| ODD59-27 | The session ignores core breakpoints | Phase 4: debugger contract |
| ODD59-28 | A dropped card file's security scope stays open while fitted | Superseded by ADR 0019 (files are imported) |
| ODD59-29 | A fresh card with an existing name asks first | Superseded by ADR 0019 (Name & Save) |

## Calc-U-1600 (`docs/background/Decisions.md`)

| Group or entry | Outcome |
|---|---|
| Emulation fidelity (`:10-249`) | Stays |
| Authentic ROM behaviour (`:259-301`) | Stays |
| Accepted limitations (`:303-328`) | Stays |
| `PC1600-Core-Limitations.md` (not modelled, per CPU and device) | Stays |
| Shared is single-threaded; coroutine jobs; web: no threads | Adopted by ADR 0009 |
| Core's machine mutexes stay (uncontended) | Superseded by ADR 0009 |
| Cards know only the bus | Adopted by ADR 0010 |
| One 60-pin connector per machine; generic open-bus path for peripheral ROMs | Stays (connector model); phase 4: peripheral and connector model |
| Preset is YAML-shaped | Adopted by ADR 0018 |
| Presets cold boot (preset owns `NEW0` and mode) | Adopted by ADR 0016 |
| `format-version` policy | Stays until the family preset format; phase 4: preset contract |
| Two preset lists (defaults, examples) | Stays as startup presets and examples (glossary) |
| `saveas:` needs `template`/`live`; name lives in the file | Stays; phase 4: media contract |
| Repository layout rules | Superseded by ADR 0002 |
| Typing rules (2-frame hold, KBII sequences, Paste never ENTER) | Phase 4: typing strategy |
| Busy state in AppSession; UI rules in Shared (`ControlState`) | Phase 4: frontend interface |
| INSPECTOR views in Core | Phase 4: debugger contract (debug panel) |
| Debugger entries (nothing copied into projects, `debug:` block, ROM extensions, bus ROM shadows, `boot: debug`, pre-execution snapshots) | Phase 4: debugger contract |
| Web: IDBFS, one writing tab, no own key hold, hit-testing in the page | Stays; phase 4: frontend components (web) |
| Loader rules (fix the loader not the preset, loaders follow MODE/`TITLE`, libsharpdx says what a file is, WAV is a program file, no line straddles module banks) | Stays |
| Host drive placement, protocol and MEP interface | Stays |
| `ApplePersistenceIgnoreState`; Copy Screen text vs `--lcd-text` | Stays |
| Wording rules (original sources only, `Ref/<path>` citations, docs describe what is, no FILEX, "original work, not a fork") | Stays; a candidate for family-wide guidance when the skills are cut (after phase 6) |

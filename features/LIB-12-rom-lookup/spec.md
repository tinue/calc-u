# LIB-12: ROM lookup

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (the descriptors list the ROM entries; a machine is
  handed its images), LIB-3 (notices).
- Contracts and decisions: `docs/contracts/roms.md` (R-1, R-4, I-1 to
  I-3, Q-1; R-2 and R-3 as amended on 2026-11-10); `docs/contracts/machine.md`
  (I-5); ADR 0034, 0035; `Briefing.md` §8 (ROMs removable: design only).
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`.

What the three status columns mean for this feature:
- **Library:** one lookup finds and checks every ROM image a descriptor
  lists among the bundled files, and a product does not start without
  its full set, shown with ROM entries and files made for the tests.
- **Calc-U-59, Calc-U-1600:** every frontend of the product and the
  harness get every ROM image through this lookup; the product's own
  loading and its own error handling are gone.

This spec owns the form of a ROM entry (AC-1) and the lists of the
products' ROM files (AC-9, AC-12); LIB-1 says that descriptors carry
such entries and points here.

What is designed but not built in the migration, and therefore not
specified here: a ROM folder of the user, a user's own dumps, and a
product that starts with some ROMs missing (`roms.md` R-2, R-3; ADR
0035). They would be needed only if the ROMs had to be taken out of the
releases (`Briefing.md` §8). AC-2 keeps the one place where they would
be added.

## User Stories
- As a user, I want the product to start with the ROMs that come with
  it, without doing anything.
- As a developer who has just cloned the repository, I want to be told
  which ROM files are missing and how to get them, instead of a machine
  that does not work.
- As the developer, I want one way to find a ROM in every frontend and
  in the harness, so that they cannot differ.

## Out of Scope
- Fetching the ROM files at build time, renaming or joining them, and
  bundling them into each frontend (ADR 0034; a step of the migration
  plan). With the migration Calc-U-59 fetches its ROMs at build time
  too; today they are committed in its repository.
- A ROM folder of the user, own dumps, a start with some ROMs missing,
  "Check again" (designed only; see above).
- What a ROM image means: decoding a text dump, mapping files to chips
  (the product's core, C59-1, C1600-1; `roms.md` R-1).
- Drawing the message of AC-5 (owners: LIB-14, C59-8).
- A bundled file that a Web frontend cannot fetch: that is a failed
  start (owner: LIB-15, AC-3 there).
- The host drive's own ROM: it is built with Calc-U-1600 and always
  bundled; the lookup is not involved (`roms.md` Q-1).
- What a module file holds besides its ROM (description, cue cards):
  `docs/formats/modules.md` and C59-6, C59-7. Since 2026-11-13 the
  module cue cards are in the module files of the ROM repository
  (AC-14).

## Acceptance Criteria

### Library
- [ ] **AC-1** — Given a ROM entry of a descriptor, then it states: the
  name the file has in the bundle, with its extension; whether the file
  is binary or is read by a text reader its product supplies; the amount
  of its decoded content, with its unit; and the md5 of the known dump
  of the file (`roms.md` R-1, R-4).
- [ ] **AC-2** — Given a ROM entry, when its image is looked up, then a
  file of exactly that name is looked for among the bundled files, in
  one place of the library that every frontend and the harness use; no
  other part of a product looks for a ROM file (ADR 0035).
- [ ] **AC-3** — Given a bundled file, then it is checked when the
  product starts by the amount of its content: a binary file by its size
  in bytes, a text file by what its reader yields, and by the checks
  its product adds (AC-8) (`roms.md` R-4). The
  md5 is checked when the files are fetched at build time, not at the
  start.
- [ ] **AC-4** — Given the product starts, then the lookup checks the
  full set: every ROM file that any model, variant or peripheral of the
  product's catalogue needs.
- [ ] **AC-5** — Given at least one file of the full set is missing or
  fails its check, then the product does not start a machine: it shows
  which files are missing or bad and that the full set is needed, and
  offers nothing else. A released product cannot get there, because
  every release bundles the full set; a local build of a freshly cloned
  repository can, before the ROMs were fetched.
- [ ] **AC-6** — Given the full set is there, then every model, variant
  and peripheral of the product can be built as far as ROMs go; no
  command is ever refused for a missing ROM while the product runs.
- [ ] **AC-7** — Given the harness and a file of the full set is missing
  or bad, then it ends with an error that names the files (LIB-11
  EC-1).

### Calc-U-59
- [ ] **AC-8** — Given Calc-U-59, then its ROM files are text dumps in
  the Datamath form, an external format, read by the reader Calc-U-59
  supplies, and checked as today: the three instruction files of a model
  together give 6144 words, its two constant files together give 64
  rows, and a module file gives at least one row of data; and the first
  and the last word of the TI-59's instruction ROM are those of the
  known dump, as today (`Shared/RomLoader.cpp:9-150`; OQ-1).
- [ ] **AC-9** — Given Calc-U-59, then its full set is these 24 files,
  with the md5 values of the files in the TI ROM repository
  (`~/Development/public/TI-59-ROM`, folders `dumps/ti59`,
  `dumps/ti58c` and `modules`): the ten calculator dumps are byte for
  byte the ones Calc-U-59 2.3.0 has in `roms/calculator/`, compared on
  2026-11-10 (`Shared/MachineModelInfo.hpp:86-105`); the 14 module files
  are those of AC-14, with the md5 values recorded when they were made
  (TI-59-ROM `90cd4a7`; `products/calc-u-59/roms/fetch_roms.sh`):

  | File | md5 | Needed by |
  |---|---|---|
  | `TMC0582.txt` | `c1cb6d40e063aae1ba7b851b11b1c18a` | TI-59, TI-58 |
  | `TMC0583.txt` | `2341a8e484c31ff24a16e70c14c1b3a3` | TI-59, TI-58 |
  | `TMC0571B.txt` | `599050d2b1c1900e64b56c089338d32e` | TI-59, TI-58 |
  | `TMC0582-CONST-K.txt` | `bf4fb6005177ffdec8f8ceee7e7e16d6` | TI-59, TI-58 |
  | `TMC0583-CONST-K.txt` | `81724f14b2827410650f032ef352b1aa` | TI-59, TI-58 |
  | `CD2400.txt` | `638c4d0c8359e00cf1f600884f43b789` | TI-58C |
  | `CD2401.txt` | `3b0ec122cd34b214477780fd405ffdd8` | TI-58C |
  | `TMC0573.txt` | `6c0cb7db542f3b74a28613ff4ed14bce` | TI-58C |
  | `CD2400-CONST-K.txt` | `dbaee54fe0f478c7659d17bff4b790d4` | TI-58C |
  | `CD2401-CONST-K.txt` | `ac81d5b45c736a0f45c3835f0d977b87` | TI-58C |
  | `ML.rom.yaml` | `ab6db47478041bb1bd1b50b3114b8949` | module `ML` |
  | `ST.rom.yaml` | `6e7e4045c9e31720dba5fbee0ace248a` | module `ST` |
  | `RE.rom.yaml` | `6ea276ed76b4b67df59141d73708e6ec` | module `RE` |
  | `SY.rom.yaml` | `d87b45b1f0fc6963db7dc3e537eeaf05` | module `SY` |
  | `NG.rom.yaml` | `889d6f5c516afd62f7bd7680cecf47ed` | module `NG` |
  | `AV.rom.yaml` | `84823786f1b065a74ba7b36319c05ba4` | module `AV` |
  | `LE.rom.yaml` | `f67efb0d1b10c46266d9a32891c9aa6c` | module `LE` |
  | `SA.rom.yaml` | `f26fd2f1cbe52ec93387c76fe26122e6` | module `SA` |
  | `BD.rom.yaml` | `4ee8f14c5ce0f088cff3ea3f0c149f76` | module `BD` |
  | `MU.rom.yaml` | `81403a7ed013e53c0334a156a7ca8c0e` | module `MU` |
  | `EE.rom.yaml` | `63fdc4324deb68025c9e98af817a91fc` | module `EE` |
  | `SE.rom.yaml` | `ea082fed55d93610d95f16cf2577f1e1` | module `SE` |
  | `AG.rom.yaml` | `4903315a721f4ff41e60b40abbaeb284` | module `AG` |
  | `RP.rom.yaml` | `d62d3ea22600d0df8f24a6355ee0ea95` | module `RP` |

- [ ] **AC-10** — Given a file of the full set is missing or bad in
  Calc-U-59, then AC-5 holds in every frontend. Today a bad machine ROM
  gives an alert while the previous machine keeps running, and a module
  that fails at the start is only a line of the log
  (`Shared/SessionController.cpp:105-111, 129-130, 220-226`).

### Calc-U-1600
- [ ] **AC-11** — Given Calc-U-1600, then its ROM files are binary
  dumps, an external format, checked by their size in bytes as today.
- [ ] **AC-12** — Given Calc-U-1600, then its full set is these 21
  files, under the names its build gives them, with the md5 values its
  build checks today (`tools/fetch_roms.sh:93-137`,
  `Core/Resources/BundledRomCatalog.hpp`). `CE-158.ROM` is joined from
  two files of the ROM repository, and the files of the PC-1600 get
  their `-new` and `-old` endings at the fetch.

  | File | Bytes | md5 | Needed by |
  |---|---|---|---|
  | `PC-1500_A01.ROM` | 16384 | `fbc55a9a8743e619b7709721ff5bcbff` | PC-1500, variant A01 |
  | `PC-1500_A03.ROM` | 16384 | `4bcf78a6d3d32e2a0349eb2d28987b8d` | PC-1500, variant A03 |
  | `PC-1500_A04.ROM` | 16384 | `8ebec8b0ef358645df14807c31df7d06` | PC-1500 variant A04; PC-1500A |
  | `CE-150.ROM` | 8192 | `eb9aa5156c6849890b137799efc50a4b` | CE-150 |
  | `CE-158.ROM` | 16384 | `aa952878fb29da4844791d95185649ca` | CE-158 |
  | `PC1600-LH5803-C000-FFFF-new.bin` | 16384 | `56168830b46d637b08529a74609bee3f` | PC-1600, variant New |
  | `PC1600-P0-B0-new.bin` | 16384 | `404bf6f2df489e09649167078acd9a25` | PC-1600, New |
  | `PC1600-P1-B0-new.bin` | 16384 | `bddbb8bbf0b2bd2d95038f67b8d002ac` | PC-1600, New |
  | `PC1600-P1-B3-new.bin` | 16384 | `6f6e1a9d46db7dc91d4322c93583ac81` | PC-1600, New |
  | `PC1600-P1-B3B-new.bin` | 16384 | `2483319acf35da4e848e59ab954abf46` | PC-1600, New |
  | `PC1600-P2-B6-new.bin` | 16384 | `86cb9036da284de2b04c7946d140a9fd` | PC-1600, New |
  | `PC1600-LH5803-C000-FFFF-old.bin` | 16384 | `6005b6420bd5e191e81a1562f3242ec9` | PC-1600, variant Old |
  | `PC1600-P0-B0-old.bin` | 16384 | `5afcc22134e106bfd63b899febe9df7c` | PC-1600, Old |
  | `PC1600-P1-B0-old.bin` | 16384 | `3bcb6b178f5967c7c8e32afe560a3e75` | PC-1600, Old |
  | `PC1600-P1-B3-old.bin` | 16384 | `ded92d8280f8f83ce3498fb9cbfb9b3d` | PC-1600, Old |
  | `PC1600-P1-B3B-old.bin` | 16384 | `2e8e075cac8f9696c5e833ceef130a4f` | PC-1600, Old |
  | `PC1600-P2-B6-old.bin` | 16384 | `2c977fdd8c924c1492a2c23f67a20f23` | PC-1600, Old |
  | `PC1600-P1-B4-CE1600P-new.bin` | 16384 | `05548a8dda3e572d50d4bd281a650ea8` | CE-1600P, variant New |
  | `PC1600-P1-B5-CE1600P-OR-F-new.bin` | 16384 | `a675c6dbdf7dc4c10e8d96891e196f8f` | CE-1600P New, and its CE-1600F |
  | `PC1600-P1-B4-CE1600P-old.bin` | 16384 | `df41b050acbc29c83214cbaaf29bee91` | CE-1600P, variant Old |
  | `PC1600-P1-B5-CE1600P-OR-F-old.bin` | 16384 | `33f3ef7207eac06587cc4c6d70c6cbd0` | CE-1600P Old, and its CE-1600F |

  Since 2026-11-13 the set also holds the module definition of the
  CE-502B, the one bundled ROM module of Calc-U-1600, with its ROM
  bytes (AC-14).

- [ ] **AC-13** — Given a file of the full set is missing or bad in
  Calc-U-1600, then AC-5 holds in both frontends, and no ROM set is used
  in place of another. Today a missing PC-1500 file or a missing file of
  the PC-1600's New set ends the Qt6 frontend with a message, also in
  the middle of a session, while a missing Old set of the PC-1600 or of
  the CE-1600P is replaced by the New one, with a notice; and the
  command-line tools look in a `roms` folder next to where they are run
  (`Shared/MachineSession.cpp:77-79, 146-158, 706-722`,
  `Qt6/app/MachineController.cpp:37-48`, `tools/pc1600_cli.cpp:110-141`).
- [ ] **AC-14** — Given a module whose file holds its ROM bytes, then
  that file lives in the product's ROM repository and is fetched and
  checked like a ROM file (AC-9, AC-12), not kept in the source
  repository (`roms.md` I-1): for Calc-U-59 the 14 Solid State ROM
  module files (`docs/formats/modules.md` §2, `.rom.yaml`), each made
  once from its dump of AC-9 and its cue cards of today's
  `roms/solid-state/cuecards.txt`, which then leaves the source
  repository; for Calc-U-1600 the module definition of the CE-502B
  (`modules.md` §1, today `Qt6/resources/cards/ce502b.card.yaml`). Their
  md5 values are recorded when the files are made or moved, and replace
  those of the 14 dumps in AC-9. Such a file is an own format: its
  version follows ADR 0037, unlike a ROM dump (`roms.md` I-3).

## Edge Cases
- **EC-1** — Given one file that two entries need (the CE-1600P's second
  file is also the CE-1600F's), then it is in the full set once and is
  named once in the message of AC-5.
- **EC-2** — Given a text ROM file with other line endings than the
  fetched one, then it is decoded and checked by its content all the
  same (`roms.md` R-4).
- **EC-3** — Given a Web frontend, whose ROMs are in its `dist/`, then a
  file it cannot fetch is a failed start of that frontend (LIB-15 AC-3).

## Technical Requirements (optional)
- Every ROM image a machine or a peripheral is handed came from this
  lookup; nothing else in a product reads a ROM file (`roms.md` I-2;
  `machine.md` I-5).
- No ROM data is in a core or in the source repository (`roms.md` I-1;
  ADR 0034, 0035).
- ROM files are external formats; ADR 0037 does not apply to them
  (`roms.md` I-3).
- A build of each product with one ROM file taken away is part of the
  tests of this feature (AC-5).
- The lookup is the only place that knows where ROM files are, so that
  a ROM folder of the user can be added there later without a change to
  its callers (ADR 0035).

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-11).* The checks Calc-U-59 has today beyond the amount: the
  first and last word of the TI-59's instruction ROM
  (`Shared/RomLoader.cpp:116-150`). *Recommendation:* keep them as they
  are; without own dumps nothing speaks against them.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| ROMs live in ROM repositories, are fetched at build time and bundled in release packages | ADR 0034 | 2026-11-09 |
| One lookup for every ROM | ADR 0035 | 2026-11-09 |
| The unit of the lookup is the file | `roms.md` R-1 | 2026-11-10 |
| An entry states the size of the decoded content; a text file is checked by what its reader yields | `roms.md` R-4 | 2026-11-10 |
| The host drive's ROM is always bundled and not looked up | `roms.md` Q-1 | 2026-11-10 |
| A ROM folder of the user, own dumps and a start with some ROMs missing are architecture only and are not specified or built in the migration; the only change is that Calc-U-59 also fetches its ROMs at build time | user, 2026-11-10, batch 3 gate; amends `roms.md` R-2, R-3 | 2026-11-10 |
| If even one file of the full set is missing, the product does not start and asks for the full set; this state is reachable only in a local build before the ROMs were fetched | user, 2026-11-10, batch 3 gate | 2026-11-10 |
| No ROM set is used in place of another; today's use of the New ROMs for missing Old ones goes | Follows from the full-set rule | 2026-11-10 |
| An entry names the file as it is bundled, not as the ROM repository names it | The Sharp build renames and joins files at the fetch (`tools/fetch_roms.sh:93-137`) | 2026-11-10 |
| The TI ROM repository is the project `~/Development/public/TI-59-ROM`; its 24 dumps are the files of Calc-U-59 2.3.0, and their md5 values are the ones of AC-9 | user, 2026-11-10; the files compared on that day | 2026-11-10 |
| Calc-U-59 keeps today's check of the first and the last word of the TI-59's instruction ROM (AC-8) | user, 2026-11-11; OQ-1 | 2026-11-11 |
| AC-9 lists the 14 module files with their md5 values instead of the 14 dumps they replace, as AC-14 says | user, 2026-11-14 (question 9 of `docs/migration-plan.md`) | 2026-11-14 |
| A module whose file holds ROM bytes lives in the ROM repository and is fetched like the ROMs: the 14 Solid State ROM modules as `.rom.yaml` (made once from the dumps of AC-9 and `cuecards.txt`), and Calc-U-1600's CE-502B module definition; `roms.md` I-1 holds, and the module file is an own format for its version (AC-14) | user, 2026-11-13 (question F-9 of `docs/walkthroughs/presets.md` §5.4); `docs/formats/modules.md` | 2026-11-13 |

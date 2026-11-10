# LIB-12: ROM lookup

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (the ROM entries of the descriptors; a machine is
  handed its images), LIB-3 (state topics, notices, the log).
- Contracts and decisions: `docs/contracts/roms.md` (R-1 to R-4, I-1 to
  I-3, Q-1); `docs/contracts/machine.md` (I-5); ADR 0034, 0035;
  `Briefing.md` §8 (ROMs removable: design only).
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`.

What the three status columns mean for this feature:
- **Library:** one lookup finds and checks every ROM image a descriptor
  lists and reports what is missing, shown with ROM entries and files
  made for the tests.
- **Calc-U-59, Calc-U-1600:** every frontend of the product and the
  harness get every ROM image through this lookup; the product's own
  loading and its own error handling are gone.

## User Stories
- As a user, I want the product to start with the ROMs that come with
  it, without doing anything.
- As a user who has dumped my own calculator's ROM, I want to use that
  dump.
- As a user of a build that comes without ROMs, I want to be told which
  files are missing and where to put them, and to go on as soon as they
  are there.
- As the developer, I want one way to find a ROM in every frontend and
  in the harness, so that they cannot differ.

## Out of Scope
- Fetching the ROM files at build time and bundling them into each
  frontend (ADR 0034; a step of the migration plan).
- What a ROM image means: decoding a text dump, mapping files to chips
  (the product's core, C59-1, C1600-1; `roms.md` R-1).
- The descriptors' ROM lists (owner: LIB-1). This feature gives the form
  of an entry (AC-1) and the Sharp table (AC-16).
- Drawing the "ROMs missing" state (owners: LIB-14, C59-8).
- What the start of a product does when the startup model's ROMs are
  missing (owner: LIB-18); "not available now" for a model, variant or
  peripheral in the control state (LIB-4).
- The host drive's own ROM: it is built with Calc-U-1600 and always
  bundled; the lookup is not involved (`roms.md` Q-1).
- The catalogue of the Solid State ROM modules, `cuecards.txt`: it is
  product material, not a ROM.
- Shipping a release without ROMs (`Briefing.md` §8: design only).

## Acceptance Criteria

### Library

Entries
- [ ] **AC-1** — Given a ROM entry of a descriptor, then it states: the
  name of the file as it comes from the ROM repository, with its
  extension; the size of its decoded content and whether that size
  counts bytes or words; and the md5 of the known dump of the file
  (`roms.md` R-1, R-4).

Finding
- [ ] **AC-2** — Given a ROM entry, when its image is looked up, then
  the bundled files are searched first and the user's ROM folder second,
  for a file of exactly that name; the first one found is taken
  (ADR 0035).
- [ ] **AC-3** — Given a machine or a peripheral is built, then every
  image it is handed came from this lookup; nothing else in a product
  reads a ROM file (`roms.md` I-2; `machine.md` I-5).
- [ ] **AC-4** — Given the same bundled files and the same ROM folder,
  then every frontend of a product and the harness find the same images.

Checking
- [ ] **AC-5** — Given a file that is binary, then it is checked by its
  size in bytes; given a file that is text, then it is decoded by the
  reader its product supplies and checked by the number of words that
  gives (`roms.md` R-4).
- [ ] **AC-6** — Given a file with the right amount of content and the
  md5 of the known dump, then it is used without a word.
- [ ] **AC-7** — Given a file with the right amount of content and
  another md5, then it is used, and a line of the log says that it is
  not the known dump (`roms.md` R-2).
- [ ] **AC-8** — Given a file with the wrong amount of content, or a
  text file that cannot be decoded, then it is not used, a message says
  why, and the entry counts as missing.

Missing ROMs
- [ ] **AC-9** — Given at least one ROM entry of the product's
  catalogue has no usable file, then the "ROMs missing" state topic
  lists each such file with its name, size, md5 and the models, variants
  and peripherals that need it, and gives the path of the ROM folder;
  given none is missing, then the topic says so (`roms.md` R-3).
- [ ] **AC-10** — Given ROMs are missing, then the product still starts;
  a model or variant whose ROMs are all there can be built and runs; a
  model, variant or peripheral that lacks one cannot be built or
  attached, and the reason names the file.
- [ ] **AC-11** — Given the "Check again" command, when files were put
  into the ROM folder in the meantime, then the lookup runs again and
  the topic and what can be built follow.
- [ ] **AC-12** — Given the harness and a machine it is asked to build
  lacks a ROM, then it ends with an error that names the files
  (`roms.md` R-3).
- [ ] **AC-13** — Given a Web frontend, then its ROMs are always
  bundled; it has no ROM folder and offers none (`roms.md` R-3).

### Calc-U-59
- [ ] **AC-14** — Given Calc-U-59, then its ROM files are text dumps in
  the Datamath form, an external format, read by the reader Calc-U-59
  supplies: for the TI-59 and TI-58 `TMC0582.txt`, `TMC0583.txt`,
  `TMC0571B.txt` and two constant files; for the TI-58C `CD2400.txt`,
  `CD2401.txt`, `TMC0573.txt` and two constant files; and one file per
  Solid State ROM module, `TMC0541.txt` to `TMC0551.txt` and
  `TMC0553.txt` to `TMC0555.txt`
  (`roms/calculator/`, `roms/solid-state/`,
  `Shared/MachineModelInfo.hpp:86-105`, `Shared/RomLoader.cpp:9-97`).
- [ ] **AC-15** — Given those files, then the amounts of content are as
  the product checks them today: 6144 words for the three instruction
  files of a model together, 64 rows for its two constant files
  together, up to 5000 bytes for a module (`Shared/RomLoader.cpp:63-150`;
  how these map to one size per file: OQ-2).
- [ ] **AC-17** — Given a ROM file is missing or bad in Calc-U-59, then
  AC-8 to AC-10 hold in every frontend. Today a bad machine ROM gives an
  alert while the previous machine keeps running, a module that fails at
  the start is only a line of the log, and in the Web frontend a failed
  fetch leaves the engine unstarted
  (`Shared/SessionController.cpp:105-111, 129-130, 220-226`,
  `Web/calc-engine-worker.js:124-150`).

### Calc-U-1600
- [ ] **AC-16** — Given Calc-U-1600, then its ROM files are these binary
  dumps, with the md5 values its build checks today
  (`tools/fetch_roms.sh:93-137`, `Core/Resources/BundledRomCatalog.hpp`):

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

- [ ] **AC-18** — Given a ROM file is missing or bad in Calc-U-1600,
  then AC-8 to AC-10 hold in both frontends and in the harness. Today a
  missing PC-1500 file or a missing file of the PC-1600's New set ends
  the Qt6 frontend with a message, also in the middle of a session; a
  missing Old set of the PC-1600 or of the CE-1600P is replaced by the
  New one with a notice; and the command-line tools look in a `roms`
  folder next to where they are run
  (`Shared/MachineSession.cpp:77-79, 146-158, 706-722`,
  `Qt6/app/MachineController.cpp:37-48`, `tools/pc1600_cli.cpp:110-141`).
- [ ] **AC-19** — Given a variant whose ROMs are missing while another
  variant of the model has its own, then the missing one cannot be
  chosen, with the reason, and no other variant is taken in its place
  without the user choosing it.

## Edge Cases
- **EC-1** — Given the same file name in the bundle and in the ROM
  folder, then the bundled one is used.
- **EC-2** — Given a file in the ROM folder whose name differs from the
  entry only in upper and lower case, then it is found on a file system
  that tells the two apart as well as on one that does not (today the
  Sharp build removes such duplicates, `tools/fetch_roms.sh:60-62`).
- **EC-3** — Given one file that two entries need (the CE-1600P's second
  file is also the CE-1600F's), then it is listed once in the "ROMs
  missing" topic, with both.
- **EC-4** — Given the ROM folder does not exist, then nothing is found
  there and nothing fails; the topic still gives its path.
- **EC-5** — Given a text ROM file with other line endings or another
  header than the known dump, then it is decoded and checked by its
  words all the same (`roms.md` R-4).

## Technical Requirements (optional)
- No ROM data is in a core or in the source repository (`roms.md` I-1;
  ADR 0034, 0035). Calc-U-59's ROM files are committed today; they move
  to a ROM repository with the migration.
- ROM files are external formats; ADR 0037 does not apply to them
  (`roms.md` I-3).
- A build of each product without any ROM file is part of the tests of
  this feature (AC-9, AC-10), although no release ships that way.

## Open Questions
- [ ] **OQ-1** — Where the user's ROM folder is. ADR 0035 says "a ROM
  folder the user chooses"; nothing says where it is by default or where
  the choice is kept, and `settings.md` leaves ROM folders to this
  contract. *Recommendation:* a folder `ROMs` next to the settings file
  by default, because the save folder of the Swift frontend is in
  iCloud; a key `[roms] folder` in the settings file for another place.
  This adds a key to LIB-9.
- [ ] **OQ-2** — The TI entries (AC-15). Today Calc-U-59 checks the
  instruction files of a model as a whole (6144 words), its constant
  files as a whole (64 rows) and a module only for having data; R-1 and
  R-4 ask for a size per file. And there are no md5 values yet, because
  the TI ROM repository does not exist (ADR 0034). *Recommendation:*
  take the word count of each file from the files of 2.3.0 when the TI
  ROM repository is made, and their md5 values with them; until then
  this column cannot be done.
- [ ] **OQ-3** — The checks Calc-U-59 has today beyond size: the first
  and last word of the TI-59's instruction ROM
  (`Shared/RomLoader.cpp:116-150`). With an md5 of the known dump and a
  size they add nothing for the known files, and they would refuse a
  user's dump of another ROM version. *Recommendation:* drop them.
- [ ] **OQ-4** — When the lookup runs: once at the start and on "Check
  again" (AC-11), or every time a machine is built? *Recommendation:* at
  the start and on "Check again"; a file removed while the product runs
  is noticed at the next start.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| ROMs live in ROM repositories, are fetched at build time and bundled in release packages | ADR 0034 | 2026-11-09 |
| One lookup for every ROM: bundled first, then the user's ROM folder; a build without ROMs starts and says what is missing | ADR 0035 | 2026-11-09 |
| The unit of the lookup is the file as fetched | `roms.md` R-1 | 2026-11-10 |
| A user's dump with the right amount of content is used even with an unknown md5, with a line in the log | `roms.md` R-2 | 2026-11-10 |
| A build without ROMs shows a "ROMs missing" state with "Check again"; the Web frontend always bundles its ROMs | `roms.md` R-3 | 2026-11-10 |
| An entry states the size of the decoded content; a text file is checked by its words | `roms.md` R-4 | 2026-11-10 |
| The host drive's ROM is always bundled and not looked up | `roms.md` Q-1 | 2026-11-10 |
| Proposed in this spec, for the gate: a ROM entry names the file with its extension (AC-1); a missing variant is never replaced by another without the user (AC-19), which ends today's quiet use of the New ROMs in Calc-U-1600 | LIB-1 left the form of an entry to this spec; R-3 says a model whose ROMs are present works and says nothing about taking another variant | 2026-11-10 |

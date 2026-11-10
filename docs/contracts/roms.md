# Contract: ROM lookup

Status: draft (2026-11-10). Forks R-1 to R-4 are decided (decision log;
R-4 was added in the phase 4 review).

Inherits: ADR 0034 (ROMs live in ROM repositories, fetched at build time
and md5-checked; release packages bundle them), 0035 (a core contains no
ROM data; the app session loads every ROM through one lookup: bundled
first, then a ROM folder the user chooses; a ROM-less build starts, says
which ROMs are missing and where they go, and boots once they are
there; designed, not shipped), `machine.md` (the descriptor lists the
ROMs a variant needs; Q-2: per chip or per file).

## Today (facts)

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| ROM files | `roms/calculator/`: TMC0582, TMC0583, TMC0571B (TI-59/58) and CD2400, CD2401, TMC0573 (TI-58C) as Datamath "ROM Recorder" text, plus `*-CONST-K.txt` constants per SCOM chip; `roms/solid-state/TMC05xx.txt`, 14 modules, with `cuecards.txt` (`roms/calculator/files.txt`) | 21 files fetched by `tools/fetch_roms.sh`, md5 per file: PC-1500 A01/A03/A04, CE-150, CE-158, PC-1600 new/old (LH5803, P0-B0, P1-B0, P1-B3, P1-B3B, P2-B6), CE-1600P new/old (B4, B5, also the CE-1600F's) (`roms/README.md`); own host-drive ROM built from `firmware/` |
| Where | committed in the repository | not in the repository; without them the build succeeds but nothing boots; release packages and `Web/dist/` bundle them |
| Missing ROM | — | `Listener::romMissing`: the Qt app reports and exits (`Shared/MachineSession.hpp:72-76`) |
| Formats | TI ROM text: external (Datamath) | binary dumps: external |

## Responsibilities

- The descriptor lists, per model and variant and per peripheral, the
  ROM images it needs (R-1).
- One library lookup finds each image: bundled, then the user's ROM
  folder (ADR 0035), and hands the bytes to the core.
- A missing image is reported with what is missing and where it goes
  (R-3).

## Non-responsibilities

- Fetching at build time (ADR 0034, build scripts).
- Interpreting ROM content (the core).

## Invariants

- I-1: No ROM data in a core or in the source repository (ADR 0034,
  0035).
- I-2: The lookup is the same in every frontend and the harness.
- I-3: ROM files are external formats; ADR 0037 does not apply to them.

## Open forks

### R-1: Unit of the lookup (machine Q-2)

**Decided (2026-11-10): option (a), the file.** Answers machine Q-2.

Options:
- (a) **Recommended:** the **file**, as fetched from the ROM repository:
  each entry has a file name, a size and the md5 of the known dump. How
  files map to chips is the core's business (the TI's constants are a
  section of the SCOM chips but separate files; the CE-1600P's B5 file is
  also the CE-1600F's ROM). This is what both products' files already
  are.
- (b) The chip: an extra mapping layer, with no user-visible gain.

### R-2: A user's own dump

**Decided (2026-11-10): option (a).**

Options:
- (a) **Recommended:** a file in the ROM folder with the expected name
  and size is used even if its md5 is not the known one, with a notice in
  the log ("not the known dump"); a known md5 is just confirmed. This is
  what "use their own dumps" (ADR 0035) needs. A file with the wrong
  size is refused with a message.
- (b) Only known md5s: a user's own dump of the same ROM version is
  refused when it differs in one byte.

### R-4: Checking a ROM file that is text

**Decided (2026-11-10, in the review; approved by the user).** Amends
R-1 and R-2 for text formats.

Facts: the TI ROM files are Datamath "ROM Recorder" text with header
lines ("TMS0500 FIRST ROM Recorder Initialized", the chip's marking, a
date) followed by address and word columns (`roms/calculator/TMC0582.txt`).
The byte size of such a file changes with line endings and header text,
so "the expected name and size" (R-2) says nothing about a user's own
dump.

Options:
- (a) **Recommended:** an entry states the size of the **decoded
  content** (words or bytes) next to the md5 of the known file. A binary
  file is checked by its byte size as before; a text file is decoded by
  the core's reader and checked by the number of words it yields. R-2
  otherwise stands: the right amount of content with an unknown md5 is
  used with a log notice, the wrong amount is refused.
- (b) Byte size for every file: a user's own text dump is refused for a
  different line ending.

### R-3: What a build without ROMs shows

**Decided (2026-11-10): option (a).**

Facts: designed, not shipped (Briefing §8, ADR 0035).

Options:
- (a) **Recommended:** the product starts; the app session sends a
  "ROMs missing" state (FE-1 topic) listing each missing file (name,
  size, md5, which model or peripheral needs it), the ROM folder's path,
  and a "Check again" action; the frontend shows it in place of the
  calculator. Models whose ROMs are present work; a peripheral whose ROM
  is missing cannot be attached. The harness exits with an error naming
  the files. The web client always bundles its ROMs (no ROM folder).
- (b) Calc-U-1600 today: report and exit.

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Machine ROM files | 3 instruction + 2 constant files per set (TI-59/58; TI-58C) | one per variant (A01/A03/A04) | 6 per variant (new/old) | CPU ROM + 4 banked ROM files (SC61860 `roms/pc1403`) | CPU ROM + BASIC ROM (`roms/pc1350`) |
| Peripheral ROM files | 14 module images (Solid State ROM modules) | CE-150, CE-158 | CE-1600P new/old (2 each), CE-150, CE-158; own host-drive ROM | — | — |
| Repository (ADR 0034) | `TI-59-ROM` (`~/Development/public/TI-59-ROM`; not yet committed or on GitHub, 2026-11-10) | tinue/PC-1500-ROM | tinue/PC-1600-ROM | tinue/SC61860 `roms/` | same |

## Open questions

- Q-1 (answered 2026-11-10): the own host-drive ROM is always bundled, built
  with the product; it is not in a ROM repository and the lookup is not
  involved. Background: Whether the own host-drive ROM (built from source) goes through
  the lookup like the others or is always bundled.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | R-1 | The lookup unit is the file as fetched (name, size, md5 of the known dump); the file-to-chip mapping is the core's. |
| 2026-11-10 | R-2 | A user's dump with the expected name and size is used even with an unknown md5, with a log notice; a wrong size is refused. |
| 2026-11-10 | R-4 | An entry states the size of the decoded content; a text ROM file is checked by the number of words its reader yields, a binary file by its byte size. Amends R-1 and R-2. |
| 2026-11-10 | R-3 | A ROM-less build starts and shows a "ROMs missing" state (files, ROM folder, "Check again"); models with ROMs work; peripherals without their ROM cannot be attached; the harness exits with an error; the web bundles its ROMs. |
| 2026-11-10 | R-2, R-3 (amended, user, phase 4b, batch 3 gate) | Architecture only, not built in the migration: the user's ROM folder, own dumps, a start with some ROMs missing and "Check again". In the migration release a product needs its full ROM set: if even one file is missing it does not start and asks for the full set, a state only a local build of a fresh clone can reach. The one change is that Calc-U-59 also fetches its ROMs at build time. |

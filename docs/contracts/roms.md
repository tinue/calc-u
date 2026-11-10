# Contract: ROM lookup

Status: draft (2026-11-10). Forks R-1 to R-3 are open.

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

Options:
- (a) **Recommended:** the **file**, as fetched from the ROM repository:
  each entry has a file name, a size and the md5 of the known dump. How
  files map to chips is the core's business (the TI's constants are a
  section of the SCOM chips but separate files; the CE-1600P's B5 file is
  also the CE-1600F's ROM). This is what both products' files already
  are.
- (b) The chip: an extra mapping layer, with no user-visible gain.

### R-2: A user's own dump

Options:
- (a) **Recommended:** a file in the ROM folder with the expected name
  and size is used even if its md5 is not the known one, with a notice in
  the log ("not the known dump"); a known md5 is just confirmed. This is
  what "use their own dumps" (ADR 0035) needs. A file with the wrong
  size is refused with a message.
- (b) Only known md5s: a user's own dump of the same ROM version is
  refused when it differs in one byte.

### R-3: What a build without ROMs shows

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
| Repository (ADR 0034) | new TI ROM repository (name open) | tinue/PC-1500-ROM | tinue/PC-1600-ROM | tinue/SC61860 `roms/` | same |

## Open questions

- Q-1: Whether the own host-drive ROM (built from source) goes through
  the lookup like the others or is always bundled.

## Decision log

| Date | ID | Decision |
|---|---|---|
| | | |

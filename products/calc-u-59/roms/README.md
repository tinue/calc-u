# ROM assets — provenance

ROMs are not in this repository (see `.gitignore`; ADR 0034). They are
Texas Instruments' copyrighted firmware. Run [`fetch_roms.sh`](fetch_roms.sh)
to (re)download the full set of 24 files into this directory before building.
It checks each file against the md5 listed in the script. All 24 lie directly
in this directory (no subfolders); every frontend, the tests and
`tools/headless/` find them through the library's ROM lookup (LIB-12,
`library/roms/`), which checks at the start that the full set is there and
names any file that is missing or bad. A build without them starts no
calculator.

Every file comes from the TI ROM repository
[tinue/TI-59-ROM](https://github.com/tinue/TI-59-ROM). Its README holds the
details of each dump (chip marking, banks, date of recording).

| Files | Source in TI-59-ROM | Needed by | Format |
|---|---|---|---|
| `TMC0582.txt`, `TMC0583.txt`, `TMC0571B.txt`, `TMC0582-CONST-K.txt`, `TMC0583-CONST-K.txt` | `dumps/ti59/` | TI-59, TI-58 | Datamath "ROM Recorder" text (external) |
| `CD2400.txt`, `CD2401.txt`, `TMC0573.txt`, `CD2400-CONST-K.txt`, `CD2401-CONST-K.txt` | `dumps/ti58c/` | TI-58C | Datamath "ROM Recorder" text (external) |
| `ML.rom.yaml` … `RP.rom.yaml` (14) | `modules/` | the Solid State ROM modules | Solid State ROM module, `docs/formats/modules.md` §2 (own, `format-version: 1`) |

The calculator dumps are byte for byte the files Calc-U-59 2.3.0 had in
`roms/calculator/`. The module files were made once from the 2.3.0 module
dumps (`roms/solid-state/TMC05xx.txt`, which are the same as TI-59-ROM
`dumps/modules/`) and from `roms/solid-state/cuecards.txt` (LIB-12 AC-14);
their ROM images are byte for byte the 2.3.0 ones (test
`lib12_ac14_module_images_equal_the_2_3_0_dumps`). Since M-24 the product
reads the module ROMs from them; the 2.3.0 dumps are no longer fetched.

`solid-state/cuecards.txt` is not a ROM file and stays in the source
repository: the module list, names and cue cards are still read from it
until M-25 (module names from the module files, C59-6) and M-55 (cue cards,
C59-7); then it goes.

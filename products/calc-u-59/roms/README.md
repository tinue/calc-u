# ROM assets — provenance

ROMs are not in this repository (see `.gitignore`). They are
Texas Instruments' copyrighted firmware. Run [`fetch_roms.sh`](fetch_roms.sh)
to (re)download the full set of 24 files into this directory before building.
It checks each file against the md5 listed in the script. All 24 lie directly
in this directory (no subfolders); every frontend, the tests and
the harness `calcu` find them through the library's ROM lookup
(`library/roms/`), which checks at the start that the full set is there and
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

The module files carry, byte for byte, the ROM images of TI-59-ROM's
`dumps/modules/` (a test pins each image's MD5) and the modules' cue
cards. The product reads the module ROMs, the module list, the names and
the cue cards from them.

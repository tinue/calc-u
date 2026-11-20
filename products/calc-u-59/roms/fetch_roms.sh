#!/bin/sh
# Downloads the TI-58/59/58C ROM dumps and the Solid State ROM module files
# Calc-U-59 needs into this directory (products/calc-u-59/roms/), from the TI
# ROM repository tinue/TI-59-ROM (see README.md here). ROMs are not in this
# repository (Texas Instruments' copyrighted firmware) -- so this
# script is how a fresh clone (or CI) gets a working roms/ directory before
# building. It is also the fetch list: names and md5 of the full set.
#
# Idempotent (tools/fetch_roms_common.sh, the repository's download and
# md5 check both products share): a file that is there with its md5 is
# skipped.
#
# Usage: products/calc-u-59/roms/fetch_roms.sh
#
# Writes the 24 files of the full set, all directly into this directory
# (no subfolders):
#   TMC0582.txt, TMC0583.txt, TMC0571B.txt, TMC0582-CONST-K.txt,
#   TMC0583-CONST-K.txt
#     -- TI-58 / TI-59, from dumps/ti59/ (Datamath "ROM Recorder" text)
#   CD2400.txt, CD2401.txt, TMC0573.txt, CD2400-CONST-K.txt,
#   CD2401-CONST-K.txt
#     -- TI-58C, from dumps/ti58c/
#   ML.rom.yaml ... RP.rom.yaml (14 files)
#     -- the Solid State ROM modules, from modules/ (docs/formats/modules.md
#        §2: ROM bytes and cue cards in one file)
#
#   TI59_ROM_BASE=<url> overrides the repository's base URL (its root, e.g.
#   a file:// URL to a local checkout).
set -eu

cd "$(dirname "$0")"
. ../../../tools/fetch_roms_common.sh

TI59_BASE="${TI59_ROM_BASE:-https://raw.githubusercontent.com/tinue/TI-59-ROM/main}"

# ---- TI-58 / TI-59 calculator ROM (dumps/ti59/) ----
fetch_if_needed "$ROMS_DIR/TMC0582.txt"         c1cb6d40e063aae1ba7b851b11b1c18a "$TI59_BASE/dumps/ti59/TMC0582.txt"
fetch_if_needed "$ROMS_DIR/TMC0583.txt"         2341a8e484c31ff24a16e70c14c1b3a3 "$TI59_BASE/dumps/ti59/TMC0583.txt"
fetch_if_needed "$ROMS_DIR/TMC0571B.txt"        599050d2b1c1900e64b56c089338d32e "$TI59_BASE/dumps/ti59/TMC0571B.txt"
fetch_if_needed "$ROMS_DIR/TMC0582-CONST-K.txt" bf4fb6005177ffdec8f8ceee7e7e16d6 "$TI59_BASE/dumps/ti59/TMC0582-CONST-K.txt"
fetch_if_needed "$ROMS_DIR/TMC0583-CONST-K.txt" 81724f14b2827410650f032ef352b1aa "$TI59_BASE/dumps/ti59/TMC0583-CONST-K.txt"

# ---- TI-58C calculator ROM (dumps/ti58c/) ----
fetch_if_needed "$ROMS_DIR/CD2400.txt"          638c4d0c8359e00cf1f600884f43b789 "$TI59_BASE/dumps/ti58c/CD2400.txt"
fetch_if_needed "$ROMS_DIR/CD2401.txt"          3b0ec122cd34b214477780fd405ffdd8 "$TI59_BASE/dumps/ti58c/CD2401.txt"
fetch_if_needed "$ROMS_DIR/TMC0573.txt"         6c0cb7db542f3b74a28613ff4ed14bce "$TI59_BASE/dumps/ti58c/TMC0573.txt"
fetch_if_needed "$ROMS_DIR/CD2400-CONST-K.txt"  dbaee54fe0f478c7659d17bff4b790d4 "$TI59_BASE/dumps/ti58c/CD2400-CONST-K.txt"
fetch_if_needed "$ROMS_DIR/CD2401-CONST-K.txt"  ac81d5b45c736a0f45c3835f0d977b87 "$TI59_BASE/dumps/ti58c/CD2401-CONST-K.txt"

# ---- Solid State ROM modules (modules/) ----
# fetch_module ID MD5
fetch_module() {
  fetch_if_needed "$ROMS_DIR/$1.rom.yaml" "$2" "$TI59_BASE/modules/$1.rom.yaml"
}
fetch_module ML ab6db47478041bb1bd1b50b3114b8949
fetch_module ST 6e7e4045c9e31720dba5fbee0ace248a
fetch_module RE 6ea276ed76b4b67df59141d73708e6ec
fetch_module SY d87b45b1f0fc6963db7dc3e537eeaf05
fetch_module NG 889d6f5c516afd62f7bd7680cecf47ed
fetch_module AV 84823786f1b065a74ba7b36319c05ba4
fetch_module LE f67efb0d1b10c46266d9a32891c9aa6c
fetch_module SA f26fd2f1cbe52ec93387c76fe26122e6
fetch_module BD 4ee8f14c5ce0f088cff3ea3f0c149f76
fetch_module MU 81403a7ed013e53c0334a156a7ca8c0e
fetch_module EE 63fdc4324deb68025c9e98af817a91fc
fetch_module SE ea082fed55d93610d95f16cf2577f1e1
fetch_module AG 4903315a721f4ff41e60b40abbaeb284
fetch_module RP d62d3ea22600d0df8f24a6355ee0ea95

log "done"

#!/bin/sh
# Downloads the Sharp firmware ROM dumps Calc-U-1600 needs into this
# directory (products/calc-u-1600/roms/), from the ROM repositories (see
# README.md here for provenance detail on each file). ROMs are not in this
# repository (Sharp Corporation's copyrighted firmware) -- so this
# script is how a fresh clone (or CI) gets a working roms/ directory before
# building. It is also the fetch list: names and md5 of the full set.
#
# Idempotent (tools/fetch_roms_common.sh, the repository's download and
# md5 check both products share): a file that is there with its md5 is
# skipped.
#
# Usage: products/calc-u-1600/roms/fetch_roms.sh
#
# Writes (22 files, into this directory):
#   PC-1500_A01.ROM, PC-1500_A03.ROM, PC-1500_A04.ROM
#     -- from tinue/PC-1500-ROM (dumps/a01/PC-1500-A01.BIN, ...; originally
#        Jeff-Birt/Sharp_PC-1500_ROM_Disassembly, Original_ROMs/)
#   PC1600-*-new.bin, PC1600-*-old.bin (6 + 6 files), plus the two
#   CE-1600P pages per version PC1600-P1-B4-CE1600P-{new,old}.bin and
#   PC1600-P1-B5-CE1600P-OR-F-{new,old}.bin
#     -- from tinue/PC-1600-ROM (dumps/new, dumps/old, dumps/ce1600p/new,
#        dumps/ce1600p/old; uppercase .BIN upstream, renamed lowercase .bin
#        with a -new/-old suffix here)
#   PC1600_ROM_BASE=<url> overrides that repo's base URL (its root, e.g. a
#   file:// URL to a local checkout).
#   CE-158.ROM
#     -- from tinue/PC-1500-ROM (dumps/ce158/CE-158-LOW.BIN + CE-158-HIGH.BIN,
#        joined; originally Jeff-Birt/Sharp_CE-158)
#   CE-150.ROM
#     -- from tinue/PC-1500-ROM (dumps/ce150/CE-150.BIN)
#   CE-502B.module.yaml
#     -- from tinue/PC-1500-ROM (modules/CE-502B.card.yaml): the CE-502B
#        module definition with its ROM bytes
#   PC1500_ROM_BASE=<url> overrides that repo's base URL (its root, e.g. a
#   file:// URL to a local checkout).
set -eu

cd "$(dirname "$0")"
. ../../../tools/fetch_roms_common.sh

mkdir -p "$ROMS_DIR"

# ---- PC-1500 and its peripherals (tinue/PC-1500-ROM) -- one directory per
# unit under dumps/: a01/, a03/, a04/ (system ROM revisions), ce150/, ce158/
# (the CE-158's two 8 KB banks as separate files, joined here); module
# definitions under modules/. ----
PC1500_REPO="${PC1500_ROM_BASE:-https://raw.githubusercontent.com/tinue/PC-1500-ROM/main}"
PC1500_BASE="$PC1500_REPO/dumps"
fetch_if_needed "$ROMS_DIR/PC-1500_A01.ROM" fbc55a9a8743e619b7709721ff5bcbff "$PC1500_BASE/a01/PC-1500-A01.BIN"
fetch_if_needed "$ROMS_DIR/PC-1500_A03.ROM" 4bcf78a6d3d32e2a0349eb2d28987b8d "$PC1500_BASE/a03/PC-1500-A03.BIN"
fetch_if_needed "$ROMS_DIR/PC-1500_A04.ROM" 8ebec8b0ef358645df14807c31df7d06 "$PC1500_BASE/a04/PC-1500-A04.BIN"
fetch_if_needed "$ROMS_DIR/CE-150.ROM" eb9aa5156c6849890b137799efc50a4b "$PC1500_BASE/ce150/CE-150.BIN"
fetch_if_needed "$ROMS_DIR/CE-158.ROM" aa952878fb29da4844791d95185649ca \
  "$PC1500_BASE/ce158/CE-158-LOW.BIN" "$PC1500_BASE/ce158/CE-158-HIGH.BIN"
fetch_if_needed "$ROMS_DIR/CE-502B.module.yaml" 7c7af4b7adab69b5e75297bc1c3d7279 "$PC1500_REPO/modules/CE-502B.card.yaml"

# ---- PC-1600 (tinue/PC-1600-ROM) -- upstream files are uppercase .BIN in
# dumps/new/ (current ROM), dumps/old/ (older ROM) and dumps/ce1600p/{new,old}/
# (CE-1600P). Renamed lowercase .bin here (the Calc-U-1600 0.8.1 Qt6 build globs *.bin,
# case-sensitive) and every ROM gets a -new/-old suffix so both versions live
# side by side in the flat roms/ directory. ----
PC1600_BASE="${PC1600_ROM_BASE:-https://raw.githubusercontent.com/tinue/PC-1600-ROM/main}/dumps"

# fetch_pc1600_calc VERSION BASENAME MD5
fetch_pc1600_calc() {
  fetch_if_needed "$ROMS_DIR/$2-$1.bin" "$3" "$PC1600_BASE/$1/$2.BIN"
}

# New ROM (PEEK #(0,&7FFF) = 4 or 5)
fetch_pc1600_calc new PC1600-LH5803-C000-FFFF 56168830b46d637b08529a74609bee3f
fetch_pc1600_calc new PC1600-P0-B0             404bf6f2df489e09649167078acd9a25
fetch_pc1600_calc new PC1600-P1-B0             bddbb8bbf0b2bd2d95038f67b8d002ac
fetch_pc1600_calc new PC1600-P1-B3             6f6e1a9d46db7dc91d4322c93583ac81
fetch_pc1600_calc new PC1600-P1-B3B            2483319acf35da4e848e59ab954abf46
fetch_pc1600_calc new PC1600-P2-B6             86cb9036da284de2b04c7946d140a9fd

# Old ROM (PEEK #(0,&7FFF) = 130)
fetch_pc1600_calc old PC1600-LH5803-C000-FFFF 6005b6420bd5e191e81a1562f3242ec9
fetch_pc1600_calc old PC1600-P0-B0             5afcc22134e106bfd63b899febe9df7c
fetch_pc1600_calc old PC1600-P1-B0             3bcb6b178f5967c7c8e32afe560a3e75
fetch_pc1600_calc old PC1600-P1-B3             ded92d8280f8f83ce3498fb9cbfb9b3d
fetch_pc1600_calc old PC1600-P1-B3B            2e8e075cac8f9696c5e833ceef130a4f
fetch_pc1600_calc old PC1600-P2-B6             2c977fdd8c924c1492a2c23f67a20f23

# CE-1600P ROMs (the two pages B4/B5 travel together; version independent of
# the calculator ROM's). New = PEEK #(5,&7FFE) 5 / 18, old = 4 / 16.
# fetch_ce1600p VERSION BASENAME MD5
fetch_ce1600p() {
  fetch_if_needed "$ROMS_DIR/$2-$1.bin" "$3" "$PC1600_BASE/ce1600p/$1/$2.BIN"
}
fetch_ce1600p new PC1600-P1-B4-CE1600P       05548a8dda3e572d50d4bd281a650ea8
fetch_ce1600p new PC1600-P1-B5-CE1600P-OR-F  a675c6dbdf7dc4c10e8d96891e196f8f
fetch_ce1600p old PC1600-P1-B4-CE1600P       df41b050acbc29c83214cbaaf29bee91
fetch_ce1600p old PC1600-P1-B5-CE1600P-OR-F  33f3ef7207eac06587cc4c6d70c6cbd0

log "done"

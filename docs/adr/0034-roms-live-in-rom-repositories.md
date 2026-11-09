# ROMs live in ROM repositories and are fetched at build time

Status: accepted (2026-11-09)

The source repository holds no ROMs. Every product fetches its ROMs at
build time from a ROM repository and checks their md5, as Calc-U-1600
does today with `tools/fetch_roms.sh`: tinue/PC-1500-ROM and
tinue/PC-1600-ROM for the Sharp models, and a new ROM repository for the
TI-58/58C/59 ROMs and Solid State ROM module images, filled from
Calc-U-59's `roms/`. Release packages bundle the ROMs (Briefing §8). A
takedown then touches only the ROM repositories and the release assets,
never the public history of the source repository.

## Considered options

- Committing ROMs under `products/<product>/roms/` (Calc-U-59 today):
  simplest, but a takedown would mean rewriting the source repository's
  public history.

## Consequences

- The TI ROM repository's name is chosen when it is created.
- `products/<product>/roms/` (ADR 0002) holds the fetch list and
  checksums, not ROM data.

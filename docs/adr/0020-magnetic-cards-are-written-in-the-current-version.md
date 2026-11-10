# Magnetic cards are written in the current version

Status: accepted (2026-11-09). For the migration release, superseded by
ADR 0037 except for "every card is written in the current version":
version 1 cards (text 1.0 and binary) are not read. After the migration
release the regular rule of ADR 0037 ("Scope") applies: the previous
card version is read and converted on the fly.

Calc-U-59 reads version 1 magnetic cards (text 1.0 and binary) forever,
and writes every card in the current card format version, as Briefing §1.2
requires for everything it writes. Today a version 1 card is written back
as text 1.0. A card written by the new release need not open in 2.3.0 or
older (no forward compatibility, §1.2).

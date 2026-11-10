# Magnetic cards are written in the current version

Status: accepted (2026-11-09); partly superseded 2026-11-10 by ADR 0037 (version 1 cards are no longer read; only the current card version is)

Calc-U-59 reads version 1 magnetic cards (text 1.0 and binary) forever,
and writes every card in the current card format version, as Briefing §1.2
requires for everything it writes. Today a version 1 card is written back
as text 1.0. A card written by the new release need not open in 2.3.0 or
older (no forward compatibility, §1.2).

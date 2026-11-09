# One product per model family; the Sharp product stays Calc-U-1600

Status: accepted (2026-11-09)

Each product emulates one model family with one core: Calc-U-59 the
TI-58/58C/59, Calc-U-1600 the PC-1500/1500A/1600. SC61860 models
(PC-1245 … PC-1475, PC-1403H, EL-5500) will become a third product, not part
of Calc-U-1600. The SC61860 shares neither CPU nor connectors (11-pin)
with the PC-1500/1600, and the user requires every product to be
releasable on its own (Briefing §6, releases); putting both model families
into one product would tie two unrelated cores to one release. The Sharp
product keeps its name Calc-U-1600, named after its most capable model like
Calc-U-59, so existing users, releases and links stay.

## Consequences

- The library must let a third product be added without restructuring
  (Briefing §3); the SC61860 product is the paper case for that in phase 5.
- What the SC61860 product shares with Calc-U-1600 (Sharp BASIC handling,
  SDE exchange, cassette tape) lives in the library, not in Calc-U-1600.
- The third product's name is chosen when it is started.

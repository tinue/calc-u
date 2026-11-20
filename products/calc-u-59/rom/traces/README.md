# ROM traces

Research data behind the ROM annotations: CPU traces (`tools/read_trace.py`
text) and the analysis that reads them, cited by `rom/*-commented.asm`,
`rom/*-labels.asm` and `docs/developer/Core-Architecture.md`.

- [sbr444-quirk-analysis.md](sbr444-quirk-analysis.md) — SBR 444 / R/S printer-interrupt quirk chain analysis; must stay in sync with `rom/TI59-commented.asm` and `examples/historic/texas-print.ti59`
- `sbr444.txt`, `rs.txt`, `pr-after-halt1.txt`, `pr-after-halt2.txt` — the printer-interrupt quirk chain (see sbr444-quirk-analysis.md)
- `errorblink.txt` — 1/x of 0 → error blink → CLR (cited in `rom/TI59-commented.asm`)

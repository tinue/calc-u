# Repository layout: one directory per product, the library beside them

Status: accepted (2026-11-09)

The source repository has `library/` for code used by more than one
product and `products/<product>/` (`calc-u-59`, `calc-u-1600`) for
everything that belongs to one product: its core, app layer, frontends,
tests, examples, ROM handling, docs, `release.yaml` and `CHANGELOG.md`.
A product directory is one release unit, so ReleaseIt can run on it
(`release -C products/<product>`); a layout by layer would scatter a
product over several trees. The specification (`docs/`, `features/`)
stays at the root; GitHub workflows must sit in the root's
`.github/workflows/`.

```
calc-u/
  CMakeLists.txt
  library/
  products/
    calc-u-59/    release.yaml CHANGELOG.md core/ app/ frontends/{swift,qt6,web}/ tests/ examples/ roms/ docs/
    calc-u-1600/  release.yaml CHANGELOG.md core/ app/ frontends/{qt6,web}/ vscode/ tests/ examples/ roms/ docs/
  docs/  features/
  .github/workflows/
```

## Considered options

- Layout by layer (`core/`, `app/`, `frontends/`, each split per product):
  rejected, because a product would not be one directory.

## Consequences

- How ROMs are handled inside `roms/` is decided separately (ROM
  removability, Briefing §8).
- The library's internal structure is decided in phase 4 with the
  contracts.

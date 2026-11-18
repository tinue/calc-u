# Calc-U

One repository for the Calc-U products: Calc-U-59 (TI-58/58C/59) and
Calc-U-1600 (PC-1500/1500A/1600), later possibly an SC61860 product. Each
product is released separately, with its own version and tags; all of them
build on one shared library.

Until both products have had their first release from this repository,
`docs/phase-f.md` applies on top of these rules. The help site is
tracked in `docs/help-site.md`.

## Layout

```
CMakeLists.txt          one root CMake project for every product
library/<area>/         the shared library, calcu_library
products/<product>/     everything of one product: release.yaml, CHANGELOG.md,
                        core/ app/ frontends/ tests/ examples/ roms/ help/ docs/
docs/                   formats/, GLOSSARY.md, Process.md, test-groups.md
tools/  ci/  .github/workflows/
```

- `calcu_library` is one static C++20 library with one folder per area
  (`machine`, `sessions`, `presets`, `media`, …). Headers and sources sit
  together, file names in PascalCase, namespace `calcu::<area>`, includes
  rooted at `library/` (`"machine/Machine.hpp"`). Other targets: the
  `calcu` harness, `calcu_web_glue`, `calcu_qt6` (Qt builds only) and the
  attach endpoint (development builds only).
- Dependencies point one way. `files` is the base; `machine`, `formats`
  and `settings` use only the standard library and `files`; `sessions`
  builds on `machine` and `pacing`; the higher areas (`presets`, `paper`,
  `debugger`, `media`, `opening`, `parts`) are used only by `harness`,
  `qt6` and the products; `web` uses only `sessions` and `settings`. A
  new include against this direction is a bug.
- Library code throws no exception across an interface and uses no RTTI,
  no threads and no locks; failures are return values. Sessions are
  single-threaded; only host adapters cross threads.
- A product's code that implements a library interface stays in the
  product (`core/`, `app/`), in the product's style.

## The products must not drift apart

- Anything that is not truly specific to one machine belongs in the
  library. Only what is truly specific stays in a product (the TI-59's
  magnetic cards, the PC-1600's own peripherals).
- A change to shared behaviour (library, a frontend structure, harness,
  help, settings, formats) is carried to every product in the same
  branch. Every product builds and tests green on `main`.
- When in doubt, it is shared. A divergence that is found is a bug.

## Working

- Working language English, for documents, code, comments and commit
  messages. Use the terms of `docs/GLOSSARY.md`.
- Short feature branches from `main`, merged with `--no-ff`; development
  and releases follow `docs/Process.md`. Pushing is fine; force-pushing
  is not.
- Tests: every test is in a group of `docs/test-groups.md` (Frequent on
  every change). Golden runs and reference runs are references: a
  difference is a finding to report, never a new reference.
- Code explains itself where it stands: a short comment that says why,
  not a pointer to another document.
- Help is Markdown in `library/help/` (shared parts) and
  `products/<product>/help/` (product parts), checked and built by
  `tools/help check|build <product>`. A user-visible change updates its
  chapter in the same branch.
- The formal specifications are binding: the file formats in
  `docs/formats/`, and the product references next to the code
  (`products/calc-u-59/reference/`, `products/calc-u-1600/docs/developer/`).
  A change to a format, protocol or interface updates its specification
  in the same branch.
- Hardware and firmware facts come from the research skills (`PC1500`,
  `SC61860`, `pc1500-build`, `ti59-research`). Don't invent facts
  (sizes, behaviour, limits); ask.
- Agent knowledge follows the code: a rename also changes the skills,
  `CLAUDE.md` files and memory that use the old name.
- The code and these rules are the current state. Earlier decisions are
  not an argument. A change is proposed as "today it is X, proposed Y";
  once agreed, the code or the rule changes, without a log of decisions.

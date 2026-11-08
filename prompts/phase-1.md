# Phase 1 — Inventory: agent prompt

Prompt for the phase 1 background agents (`docs/Process.md`, phase 1). Each
agent reads this whole file, then does only the part assigned to it.

## Context

Calc-U merges two emulator products, Calc-U-59 (TI-59) and Calc-U-1600
(Sharp PC-1500/1600), into one repository with a shared layer. We are in
the specification phase. Phase 1 collects **facts only**: how each product
does things today. Decisions come in phase 3.

Read first, completely:
- `/Users/me/Development/public/Calc-U/Briefing.md`
- `/Users/me/Development/public/Calc-U/docs/Process.md` (phase 1 section)
- `/Users/me/Development/public/Calc-U/CLAUDE.md`

## Rules for every agent

1. **Read only, at the pinned tag.** Do not change any file in a product,
   site, tooling or research repository. Do not `git checkout`, `switch`,
   `stash`, `reset`, `commit` or `fetch`-and-merge. The working trees differ
   from the tags and contain uncommitted work: read pinned content with
   `git show <tag>:<path>`, `git ls-tree -r <tag>`, `git grep <pattern>
   <tag>`. Pins:
   - Calc-U-59: `/Users/me/Development/public/Calc-U-59`, tag `v2.3.0`
     (`231792c`).
   - Calc-U-1600: `/Users/me/Development/sharp/Calc-U-1600`, tag `v0.8.0`
     (`d976d3f`).
   - calc-u-site and ReleaseIt: current `HEAD`; record the SHA in the
     header.
2. **Do not build or run** anything (no CMake, Xcode, npm, tests). Elapsed
   times of tests come from GitHub Actions run logs (`gh run list`,
   `gh run view --log`, read-only) or are recorded as unknown.
3. **Facts, not opinions.** No recommendations, no "should". The only
   exception is the backlog target column, which is a *suggestion*.
4. **Every fact has a source:** `path:line` at the pinned tag (or the
   recorded SHA). Facts you could not verify are marked *unverified*.
5. **Don't invent.** If something cannot be found, write it under "Open
   questions" in your file.
6. **English** only. Use the briefing's terms; there is no glossary yet.
   Where a codebase uses its own name for a concept, give that name in
   backticks.
7. **Hardware and firmware facts** come from the research skills and
   repositories (Briefing §5), not from memory.
8. Write only the files assigned to you, under
   `/Users/me/Development/public/Calc-U/docs/inventory/`. Do not commit.
9. End with a short report to the orchestrator (not the files' content):
   files written, the five most important findings, open questions,
   anything that contradicts the briefing.

## File template (product inventories)

```
# Inventory: <product>

Pinned: <tag> @ <sha> (= <dev branch> @ <sha>). Read on <date>.

## Summary
<10 lines at most>

## Capabilities
### <capability>
- How it works today: …
- Where it lives: `path` (…)
- Frontends: Swift / Qt6 / Web — which have it, differences
- Maturity: complete / partial / stub / missing, with evidence
- Product-specific names: `…`

## <further sections as assigned>

## Open questions
- …
```

## Agent A — Calc-U-59

Write `docs/inventory/calc-u-59.md` and `docs/inventory/backlog-calc-u-59.draft.md`.

`calc-u-59.md` covers:
- **Capabilities:** keyboard, presets, debugger, settings, media/files
  (cards, Solid State Software modules), printer (PC-100C), audio, pacing,
  threading, display, web shell, tests/headless, help-site material, and
  any capability not on this list.
- **Architecture overview:** layers (core, shared, frontends), their
  directories, languages and build (CMake, Xcode, web).
- **Modelling depth (Briefing §1.9):** which TI-59 chips or blocks are
  modelled separately and which are folded together; which interfaces are
  explicit or implicit (printer connector, module slot); where functional
  emulation is used. Why `examples/assembly/Decoder.asm` does not run:
  what exactly it depends on. Use `ti59-research`
  (`/Users/me/Development/ti59-research`) and the repository's `rom/` and
  `reference/`.
- **Compatibility surface (Briefing §1.2):** every own and external file
  format the release reads and writes. For each own format: is it
  explicitly versioned, which versions are read, which version is written.
  Unversioned legacy formats still accepted are listed as *dropped* (Briefing
  §1.2). List every bundled file and every file hosted on calc-u.org
  (`/Users/me/Development/public/calc-u-site`, `/59/`) that is in an
  unversioned legacy format. Tag every format own or external.
- **GUI concepts (Briefing §1.3, §1.5):** calculator, printer, debugger,
  preset loader, card loader, module loader, file load and save, bundled
  material. Per frontend (Swift iOS, Swift macOS, Qt6, Web): where it sits
  in the GUI and how it is reached.
- **Tests (Briefing §1.10, §8):** every test harness, headless emulator and
  test suite: kind (unit, GUI, golden, …), platform, where it runs (locally,
  GitHub Actions), elapsed time if known, capabilities (headless run,
  scripted input, state dump, golden tests, …).
- **Deliberate oddities:** from `reference/` and similar: behaviour that
  looks wrong but is intended, with its reason, so it survives the merge.
- **Release material:** App Store screenshots, `appstore` branch and
  directory, what they contain.

`backlog-calc-u-59.draft.md`: every item of `TODO.md` and of each document
in `ideas/` at `v2.3.0`. One row per bullet or sub-bullet that is its own
idea; headings and pure context lines get no row. Table columns:
`ID` (`B59-1`, `B59-2`, … in source order) | `Source` (`TODO.md:12` or
`ideas/Pico.md:3`) | `Item` (short, faithful) | `Suggested target`
(design input / shared backlog / product backlog / done / dropped) |
`Reason` (one line; required for done and dropped, with evidence for done).

## Agent B — Calc-U-1600

Write `docs/inventory/calc-u-1600.md` and `docs/inventory/backlog-calc-u-1600.draft.md`.

`calc-u-1600.md` covers the same sections as agent A, adapted:
- **Capabilities:** keyboard, presets, debugger (including DAP), settings,
  media/files (memory modules/cards, floppy, host drive, cassette tape,
  serial port), printer (CE-150, CE-1600P), audio, pacing, threading
  (coroutine jobs), display, web shell, tests/headless, help-site material
  (`docs/`), SDE integration (vendored `libsharpdx`, CI artifact pull), and
  anything not on this list. Cover both machines (PC-1500, PC-1600) and how
  they share code.
- **Architecture overview** as for agent A.
- **Modelling depth (Briefing §1.9):** per machine, which chips are modelled
  separately (LH5801/LH5803, Z-80, SC7852, …), which are folded together,
  which interfaces and expansion connectors are explicit, where functional
  emulation is used. Use the `PC1500` research skill and its repositories.
- **Formats (Briefing §1.2):** every own and external format, read and
  written. For each own format: which ones already carry a version number,
  and which version. Tag every format own or external.
- **GUI concepts, tests, deliberate oddities** as for agent A. Oddities
  come from `docs/background/Decisions.md` and similar.

`backlog-calc-u-1600.draft.md`: every item of `TODO.md` at `v0.8.0`
(728 lines), same rules and columns as agent A, IDs `B1600-1`, ….

## Agent C — calc-u-site, release tooling, agent knowledge

Write `docs/inventory/calc-u-site.md`, `docs/inventory/release-tooling.md`
and `docs/inventory/agent-knowledge.md`.

`calc-u-site.md` (`/Users/me/Development/public/calc-u-site`, current
branch and `HEAD`, both recorded): site structure and build, material per
product (`/59/`, `/1600/` placeholder), content duplicated or near-duplicated
between products, how the web clients are embedded and where they come
from, the redirect from calcu59.ch, the stale README (what it says vs. what
is true). Downloadable files hosted on the site: list them with their
format; agent A checks which are legacy.

`release-tooling.md`: ReleaseIt (`/Users/me/Development/public/ReleaseIt`):
how it works, every single-product assumption (one root `release.yaml`,
`vX.Y.Z`/`-pre` tags, `dev-x.y.z` branches, one CHANGELOG, …) with source.
Both products' `release.yaml` and `.github/workflows/*.yml` at their pins:
what is built where (locally vs. GitHub Actions), signing, notarisation,
the App Store pipeline, the SDE artifact pull, artifacts per release.

`agent-knowledge.md` (Briefing §1.11): every element of agent guidance and
memory that must be updated once the glossary exists. One table row per
element: path, kind (skill, `CLAUDE.md`, memory file, agent-facing
developer doc), owner (Calc-U-59, Calc-U-1600, research, global), size,
and the domain terms it uses most (preset, card, module, session, machine,
…). Cover:
- product skills and `CLAUDE.md` files of both products, at their pins;
- agent-facing docs (for example Calc-U-1600 `docs/developer/`);
- global skills in `~/.claude/skills/` (`PC1500`, `SC61860`,
  `pc1500-build`) and the TI skills mentioned in Briefing §5.1;
- Claude memory directories under `~/.claude/projects/` that belong to
  these repositories (names and sizes only; read them to list their terms).

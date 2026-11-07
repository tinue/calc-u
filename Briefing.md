# Calc-U — Briefing

One repository for the Calc-U emulators (Calc-U-59, Calc-U-1600, and later
possibly an SC61860 product). The emulators still ship as separate products,
each on its own release cycle. The goal is to remove duplication and drift in
the features the products have in common.

## 1. Goals

### 1.1 Products keep working
- Calc-U-59 and Calc-U-1600 continue to be released, without breaking any
  user-visible feature.
  - Each emulator keeps the same familiar GUI with the same functionality.
- Each product keeps its own release cycle and its supported platforms (§5.2).
- Each product keeps its own release material, such as App Store screenshots.
- "The same GUI" means the same layout, assets, controls and behaviour,
  judged by the developer in side-by-side use. It does not mean
  pixel-identical. Screenshots are retaken only where something visible
  changed.

### 1.2 Supporting files (presets, cards, floppies, state files, …)
- These rules apply to Calc-U's **own** formats: presets, state files,
  settings and its own card/floppy containers. **External** formats (from
  the real machines or outside tools: tape WAV, ROM dumps, SDE exchange
  formats, raw BASIC/binary images) must never break, in either product.
  The inventory tags every format as own or external.
- **Calc-U-59 stays backwards compatible.** Anything it reads may be in the
  old format. Anything it writes uses the new format. Writing an item that
  was just read therefore upgrades it.
  - "Old format" means every format the latest *released* Calc-U-59 reads,
    including legacy formats it already accepts, the files bundled in the
    app and the files hosted on calc-u.org.
  - No forward compatibility: an upgraded file need not open in an older
    Calc-U-59.
- **Calc-U-1600 needs no backwards compatibility.** Files saved by 0.8.0 may
  not load in the first release from this repository. A release-note
  warning is enough; no converter.

### 1.3 One source of truth for shared features
- Features that both products have (for example presets or keyboard support)
  have exactly one implementation, in the shared layer.
- **What stays per product:**
  - The calculator core.
  - Product material: faceplates, icons, the keyboard hit map and similar.
  - The GUIs themselves. Code duplicated between them moves to the shared
    layer (for example the preset loader).
- **Shared layer vs. product scope.** A product uses the shared layer for a
  feature, but it only exposes as much of that feature as it supports today.
  Example: keyboard support has a shared mechanism plus a per-product
  definition of which host key does what on the emulator. Calc-U-59 must use
  the shared keyboard layer, but its key definitions stay at what Calc-U-59
  supports today. Extending them is not a goal of this project. The same
  applies to the DAP debugger: Calc-U-59 does not have to gain it.

### 1.4 Robust abstractions
- The abstractions between the layers keep redundancy and feature drift to a
  minimum.
- They are good enough to support, later:
  - adding a new GUI target (for example a Swift frontend for Calc-U-1600);
  - adding a new core (for example SC61860 machines in Calc-U-1600).
- Designing for these is a goal. Building them is not (§2).

### 1.5 Coherent for end users
- Vocabulary is the same across all emulators: a preset means the same thing
  everywhere.
  - A glossary term may rename a visible label, menu item or help text in a
    product. This counts as "same functionality" (§1.1). Every rename is
    listed in the phase 2 mapping table.
- The help sites are coherent. Common material is shared, not duplicated.

### 1.6 SC61860 preparation
- Prepare for later support of SC61860-based machines such as the PC-1403H.
  Whether they become part of Calc-U-1600 or a third product is open (§7).

### 1.7 Existing backlogs are carried over
- Both TODO files (§5.4) are reviewed item by item. Items relevant to the
  shared layer or the abstractions become part of the design.
- No idea gets lost. Every idea listed there is kept, either in the shared
  backlog or in the backlog of one product, even if this project doesn't
  implement it.

### 1.8 Guiding spirit
- One codebase, several products. PockEmul and MAME are the model for the
  first part (one project), not for the second: they ship one product, Calc-U
  ships several.

## 2. Non-goals

- Frequent releases of Calc-U-59 or Calc-U-1600 during this project.
- One single emulator product.
- Redesign or rework of the research projects. TI, PC-1500/1600 and SC61860
  keep their knowledge bases and skills (§6).
- Integration of SDE (SharpDataExchange). It stays a separate project and is
  pulled in where needed.
- Integration of ReleaseIt. It stays a separate project, but it is an
  upstream dependency like SDE: it may be extended in its own repository
  where releasing several products from one repository needs it.
- Adding or removing build targets. Every existing target stays, including
  the Calc-U-1600 web client. The design must allow new targets later
  (§1.4), but this project adds none.
- Mixing families that are too far apart. TI might later gain a TI-57 core,
  but an HP-41 core would be a separate product (Calc-U-41).
- Extending product features beyond today's set, for example DAP for
  Calc-U-59 or more host-key definitions (§1.3).

## 3. Decisions already made (not to be reopened)

- **One repository**, initialised in this directory. On GitHub it is
  `tinue/calc-u`, initially private.
- **Products:** two products as they are today, Calc-U-59 and Calc-U-1600.
  The design must allow adding a third product (for example for SC61860
  machines) without restructuring.
- **calc-u-site stays its own repository.**
- **Hotfixes:** until development in this repository starts, urgent fixes
  may still be released from the existing repositories. Once development
  starts, no more hotfixes are allowed.
  - "Development starts" means the first commit in this repository that
    imports product code (phase 6, step 1).
  - Until then, every hotfix in an old repository is carried into the
    inventory as a delta against its pinned baseline.
- **History:** the full history of both repositories must be archived (the
  mechanism is open, §7).
- **Calc-U-1600 baseline:** all imports, inventory and decisions use the
  branch `dev-0.8.0`.
- **Working language is English**, for specs, design documents, code,
  research documents and skills. Original source material (PDFs, manuals) may
  be in other languages.

## 4. Discussed, but not yet decided

Moved to §7.

## 5. Starting point

### 5.1 Both repositories
- `main` is squashed. The full history only exists in the various
  `dev-x.y.z` branches.
- Few users: fewer than 1000, possibly fewer than 100. A niche product for
  retro enthusiasts.
- Both were built for personal use first and then shared, because such
  emulators are rare and the existing ones are not very good. The few users
  regard both as very good, better than any other emulator around.
- Both are released with ReleaseIt (`/Users/me/Development/public/ReleaseIt`);
  see `release.yaml` in each repository.

### 5.2 Products and platforms

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Repository | `/Users/me/Development/public/Calc-U-59` (`tinue/Calc-U-59`) | `/Users/me/Development/sharp/Calc-U-1600` (`tinue/Calc-U-1600`) |
| State | Released; nothing substantial on the current dev branch | `dev-0.8.0` close to release; to be released before full work on Calc-U starts |
| Core / shared layer | C++17, CMake | C++20, CMake |
| Release tooling | ReleaseIt: Swift macOS app built and notarised locally, everything else from GitHub Actions (`build.yml`, dispatched) | ReleaseIt: everything from GitHub Actions (`build.yml`, dispatched); nothing built locally |

| Product | Frontend | Platform | Minimum version | Distribution | Status |
|---|---|---|---|---|---|
| Calc-U-59 | Swift / SwiftUI | iPhone, iPad | iOS 18 | App Store (`ch.erzberger.calcu59`), TestFlight | Released |
| Calc-U-59 | Swift / SwiftUI | macOS | macOS 15.6 | DMG on GitHub (Developer ID, notarised) | Released |
| Calc-U-59 | Qt6 (6.8.2 in CI) | Windows x86_64 / arm64 | — | Installer on GitHub (unsigned) | Released |
| Calc-U-59 | Qt6 (6.8.2 in CI) | Linux x86_64 / aarch64 | — | AppImage on GitHub | Released |
| Calc-U-59 | Qt6 | macOS | macOS 15.6 (CMake) | Not shipped; local builds only | Local only |
| Calc-U-59 | Web (React + WASM) | Browser | — | Zip on GitHub, embedded by calc-u.org | Released |
| Calc-U-1600 | Qt6 | macOS arm64 | macOS 15.8 (see note) | DMG on GitHub (Developer ID, signed in CI) | Released |
| Calc-U-1600 | Qt6 (6.8.2 in CI) | Windows x86_64 / arm64 | — | Installer on GitHub (unsigned) | Released |
| Calc-U-1600 | Qt6 | Linux x86_64 / aarch64 | — | AppImage on GitHub | Released |
| Calc-U-1600 | Web (WASM) | Browser | — | Zip on GitHub | Released, not yet published (see below) |

The Calc-U-1600 web client is built as a zip and attached to each release.
It is not published on calc-u.org yet, because the Calc-U-1600 help site
does not exist yet (§5.3).

Note: macOS 15.8 is set consistently in CMake (`CMAKE_OSX_DEPLOYMENT_TARGET`),
in SDE (`.cargo/config.toml`, checked by its release workflow) and by the CI
runner (`macos-15`). Finding for the inventory: the vendored `libsharpdx.a`
on `dev-0.8.0` was built locally, so its prebuilt Rust `std` objects carry
`minos 27.0`. They link with warnings only, but make 15.8 untested in
practice. The fix is to vendor the library from an SDE release (rustup std,
macOS 11). It ships with Calc-U-1600 0.8.0, from the old repository.

Calc-U-1600 also pulls SDE (SharpDataExchange) release artifacts into its CI
builds.

### 5.3 calc-u-site
- Repository: `/Users/me/Development/public/calc-u-site`. Builds
  https://calc-u.org (documentation; it also embeds the web clients).
- The site must always reflect a released product, so it is in scope, but it
  stays a separate repository (§3).
- `/59/` is the full Calc-U-59 site and its only source. calcu59.ch serves
  only a redirect and will keep doing so for some time; the old `gh-pages`
  branch of Calc-U-59 is deactivated and deleted. (The calc-u-site README
  still describes the old setup.)
- `/1600/` is a placeholder. Building the Calc-U-1600 help site is in scope:
  it starts from Calc-U-1600's `docs/`, uses the shared-material structure,
  and publishes the Calc-U-1600 web client. This is new documentation, not a
  new product feature (§2).

### 5.4 Backlogs

| Source | Size | Content |
|---|---|---|
| `Calc-U-59/TODO.md` | 35 lines | Core emulation, settings, UI, CPU and calculator debugger, various, "far out" ideas |
| `Calc-U-59/ideas/` | 5 documents | `CardRecovery`, `CardSound`, `KeypressLatch`, `Pico`, `PresetConvergence` |
| `Calc-U-1600/TODO.md` (`dev-0.8.0`) | 728 lines | Known issues, LH5803/SC7852 interface, serial port, loading, expansion connectors ("one model on both machines"), cassette tape, 1.0 announcement, shared layer and web GUI, feature ideas, code cleanup backlog |

## 6. Research base (unchanged by this project)

### 6.1 Global research skills

| Skill | Covers | Points to |
|---|---|---|
| `PC1500` | PC-1500/1500A/1600 hardware, firmware, ROM disassemblies, BASIC | `Sharp1500-1600-Ref`, `SharpBasicReference`, `pc1600`, `PC-1600-ROM`, `pc1500`, `PC-1500-ROM` |
| `pc1500-build` | Assembling LH5801 sources to `.bin` | `sdcc-pc1500` |
| `SC61860` | SC61860 machines: CPU, models, 11-pin port, tape and BASIC formats, ROMs | `SC61860` |
| (no research router) | TI-58/59: TMC0501, ROM, PC-100C | `ti59-research`; plus `rom/` and `reference/` inside Calc-U-59. TI knowledge is spread over several smaller skills; a router to the PDFs (like `PC1500`/`SC61860`) does not exist. Accepted as is. |

### 6.2 Product skills (to be restructured, §4)

| Product | Skills |
|---|---|
| Calc-U-59 (`.claude/skills/`) | `calc-u-59-core`, `-shared`, `-swift`, `-qt`, `-web`, `-docs`, `-statefiles`, `-uitesting`, `pre-release-cleanup` |
| Calc-U-1600 | None as skills; guidance lives in `CLAUDE.md` and `docs/developer/` |

### 6.3 Repositories (all under `~/Development/`)

| Path | GitHub | Content |
|---|---|---|
| `ti59-research` | `tinue/ti59-research` | TI-59 research: architecture, quirks, ROM disassembly, PC-100C |
| `sharp/Sharp1500-1600-Ref` | `tinue/Sharp1500-1600-Ref` | Consolidated PC-1500/1600 reference |
| `sharp/SharpBasicReference` | `tinue/SharpBasicReference` | PC-1500/1600 BASIC references |
| `sharp/pc1500` | `tinue/sharp-pc1500` | PC-1500 ROM annotation, LH5801 disassembler toolchain |
| `sharp/PC-1500-ROM` | `tinue/PC-1500-ROM` | PC-1500, CE-150 and CE-158 ROM dumps and disassemblies |
| `sharp/pc1600` | `tinue/pc1600` | PC-1600 ROM annotation project |
| `sharp/PC-1600-ROM` | `tinue/PC-1600-ROM` | PC-1600 and CE-1600P ROM dumps and disassemblies |
| `sharp/SC61860` | `tinue/SC61860` | SC61860 knowledge base, ROMs, disassemblies |
| `sharp/sdcc-pc1500` | `pchambre/sdcc-pc1500` (third party) | sdas/sdld toolchain for the LH5801 |
| `sharp/SharpDataExchange` | `tinue/SharpDataExchange` | SDE: data exchange with real hardware (stand-alone, §2) |

### 6.4 NAS directories (`~/SynologyDrive/Dokumente/PDF/Vintage/`)

| Directory | Content |
|---|---|
| `TI/` | TI manuals, `TI-57/`, PPC notes, PPX Exchange, Solid State ROM material |
| `Sharp/PC-1500/`, `Sharp/PC-1600/`, `Sharp/CE-158X_Users_Manual/`, `Sharp/CE163x_material/` | PC-1500/1600 and peripheral manuals |
| `Sharp/SC61860/` | SC61860 documents (about 230 files, 1.4 GB; indexed in `SC61860/research/`) |
| `Sharp/` (other) | Newsletters, magazines, textbooks, general Sharp material |

## 7. Open points (for grilling and the spec phase)

Each point names the phase that settles it (`docs/Process.md`).

- SC61860: part of Calc-U-1600 or a third product; the name of the Sharp
  product. → Phase 3.
- How the full history of both repositories is archived. → Phase 3.
- The core seam: "The machine is the only core seam; composition instead of
  family cores." The discussion was not deep enough yet and needs more
  thought. → Phase 3.
- Skills: one set of skills per product, plus a shared skill. Structure
  open. → Phase 3.
- How calc-u-site shares common material across products. → Phase 3/4.
- Where releases are published after the move (new repository or the old
  ones), and what happens to the old repositories. → Phase 3.
## 8. Success criteria

- One release each of Calc-U-59 and Calc-U-1600 is live, built from the new
  repository. Calc-U-59 is also live in the App Store.
- Each product looks the same as before (§1.1) and supports at least its
  current set of features.
- Each product uses the new shared layer for every feature listed as shared.
  The list is `features/INDEX.md` (the KIT-n entries).
  A product that still has its own implementation of a shared feature does
  not meet this criterion.
- Calc-U-59 reads all of its existing files and writes the new format (§1.2).
- calc-u.org reflects both releases and shares common material between the
  products instead of duplicating it. This includes the new Calc-U-1600 help
  site with the published web client (§5.3).
- A design for SC61860 support exists and has been validated against the
  abstractions: adding it needs no changes to the shared layer.
- Every item of both TODO files (§5.4) is accounted for: it is part of the
  design, or it is in the shared backlog or a product backlog, or it is
  recorded as done or dropped, with a reason.
- New product features are not required (§1.3, §2).

## 9. Constraints

- One developer, working with heavy Claude Code support.
- No time constraints.
- No commercial constraints: this is a hobby project.
- License stays GPL-3.0. Released products contain ROM files, with a
  takedown notice in this repository.
  - The design must allow removing the ROMs, with the user adding them after
    downloading the product. Design only; do not implement.
- Keep the existing language versions (C++, Swift, Xcode, Qt). Always
  consider whether they can be upgraded to stay current. Versions may be
  raised, never lowered; every raise is an ADR (for example Calc-U-59 from
  C++17 to C++20). The minimum OS versions below stay fixed.
- Swift app: iOS 18 and macOS 15 or later; keep this for now.
- Qt6 macOS app (Calc-U-1600): macOS 15.8 or later; keep this for now (see
  §5.2 note).
- Each artifact keeps its current build location: the Calc-U-59 Swift macOS
  app is built and notarised locally, everything else on GitHub Actions. No
  cross-compile scaffolding in this repository.
- A local build must always be possible, for rapid iteration.
- The test harnesses and headless emulators must keep working, for rapid
  iteration by an AI agent. Their capabilities must stay (headless run,
  scripted input, state dump, golden tests, …); their interfaces may change
  if the skills and docs that use them change in the same step.

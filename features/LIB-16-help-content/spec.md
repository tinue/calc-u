# LIB-16: Help content

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: none that must be built first. The help describes what the
  other features do; each chapter names the specs it follows (AC-3), and
  a chapter is finished when those are.
- Contracts and decisions: `docs/contracts/help.md` (HL-1 to HL-3, I-1
  to I-3); ADR 0024, 0036; Briefing §1.5, §4.3;
  `docs/glossary-mapping.md` (visible renames).
- Facts: Calc-U-59's help in calc-u-site at `acd629c` (`59/Pages.jsx`,
  `59/routes.js`); Calc-U-1600's `docs/` at `b4ea8c8`.

Words used here: the **help** of a product is its user guide; a chapter
has a **shared part**, written once for the family and holding nothing
of one product, a **product part**, which belongs to one product, or
both; the **help asset** is the file `<Product>-help.zip` that a
release carries; the **help tool** is the command of the source
repository that builds and checks a help asset (AC-6).

What the three status columns mean for this feature:
- **Library:** the shared parts are written, and the help tool builds
  and checks the help asset of any product. The tool is this feature's
  only code.
- **Calc-U-59, Calc-U-1600:** the product's chapters are written, and
  its release carries its help asset.

## User Stories
- As a user of both products, I want what is the same in both to be
  explained once and in the same words.
- As a user, I want the help I read to describe the release I have.
- As the author of the help, I want to write Markdown next to the code
  and find a broken link before a release.

## Out of Scope
- Showing the help: the site that renders the asset (owner: LIB-19); the
  Help menu item that opens it (LIB-14 AC-20).
- Developer documentation, which stays with the code and is not help
  (`help.md`, non-responsibilities).
- The site's design and its other pages (calc-u-site; OQ-1).
- Help inside the product, without a browser (a new function).
- Translations.

## Acceptance Criteria

### Library

Structure
- [ ] **AC-1** — Given the help of a product, then it has these ten
  chapters in this order, each with a shared part (S), a product part
  (P) or both (`help.md` HL-2):

  | # | Chapter | Parts | Covers |
  |---|---|---|---|
  | 1 | Introduction | P | the product, its models, what is emulated |
  | 2 | Getting started | S, P | the window, the control bar and the menus (S); the faceplate and the keyboard map (P) |
  | 3 | Models and machine control | S | model, variant, Reset, Cold boot, the power cycle after attaching, ROMs |
  | 4 | Media | S, P | templates, instances, Name & Save, insert and eject, outdated files (S); the product's media (P) |
  | 5 | Peripherals and paper | S, P | printers, the paper panel (S); the product's devices (P) |
  | 6 | Presets | S, P | the format, the steps, startup presets (S); the product's sections and examples (P) |
  | 7 | Debugger | S, P | pause, step, resume, the debugger steps of a preset (S); the product's debugger (P) |
  | 8 | Settings | S, P | the settings file, where it lies, the keys of the family (S); the product's keys (P) |
  | 9 | Product topics | P | what only this product has |
  | 10 | Reference | S, P | the file formats in their current versions, the keyboard maps, the glossary |

- [ ] **AC-2** — Given a file of a shared part, then it names no
  product, no model and no peripheral of one product, and it has no
  picture of a product's window, only diagrams; what differs by model
  (what a Reset keeps, for one) stands in a product part (`help.md`
  HL-1).
- [ ] **AC-3** — Given the shared part of a chapter, then it describes
  what these specs say, in the glossary's terms: 2: LIB-14, LIB-5; 3:
  LIB-18, LIB-12; 4: LIB-7, LIB-13; 5: LIB-2, LIB-8; 6: LIB-6; 7:
  LIB-10; 8: LIB-9; 10: the grammar of the preset format (LIB-6 AC-1)
  and the rules of the settings file (LIB-9), each once. A product part
  describes what the product's rows say, and where a thing has no row
  because it is carried over unchanged (the host drive, the COM ports,
  fast mode), what the product's document of today says (AC-9, AC-11).
- [ ] **AC-14** — Given chapter 8, then every key the settings of a
  product declare (LIB-9 AC-19, AC-26) is described there, the family's
  in the shared part and the product's in its part; the help tool fails
  when a declared key is not described.
- [ ] **AC-4** — Given the help, then it is written in Markdown, in
  English, with pictures as files next to it and diagrams in Mermaid
  where a diagram helps (ADR 0036; `help.md`, facts).
- [ ] **AC-5** — Given the help, then it uses the glossary's terms and
  the visible names of `docs/glossary-mapping.md`; the help tool lists
  every place where a word stands that the glossary gives under "Avoid",
  as a warning for the author to judge (`help.md` I-3; OQ-3). The
  glossary of chapter 10 holds the terms the help itself uses.

The asset
- [ ] **AC-6** — Given a product and a version, then the help tool
  builds its help asset `<Product>-help.zip` from the source repository:
  the Markdown files of its ten chapters, the shared parts and its own,
  its pictures, the files its text offers for download (the presets of a
  tutorial), and a table of contents that lists the chapters in order,
  each with its title, its files and whether a file is shared, and that
  names the product and its version (`help.md` HL-3; OQ-2).
- [ ] **AC-7** — Given the help tool builds an asset, then it fails, and
  says where, when: a chapter of AC-1 is missing; a link to another help
  file, to a heading or to a picture leads nowhere; a file of the help's
  source folders is not in the table of contents; a picture is used by
  no file; or a file of a shared part names a product, a model or a
  peripheral of one product (AC-2).
- [ ] **AC-15** — Given a link in a help file, then it is written
  relative to that file when it leads into the same product's help,
  with the heading's anchor made by the rule GitHub uses for Markdown
  headings, and as a full `https://` address otherwise; LIB-19 renders
  by the same rule.
- [ ] **AC-8** — Given the release of a product is built, then the help
  tool builds the help asset of that version, and the asset is among the
  release's files; the test suite builds both products' assets
  (`help.md` HL-3, I-1).

### Calc-U-59

- [ ] **AC-9** — Given the help of Calc-U-59, then its product parts are
  written from the pages the site has today, converted once (ADR 0036):

  | Today's page (`59/routes.js:24-39`) | Goes to |
  |---|---|
  | `/` (Home), `/what-is-a-ti-59/` | 1 Introduction |
  | `/getting-started/` and its topics, one of them the printer and the card reader | 2, 4 and 5 |
  | `/install/…` (four pages) | 2, product part (OQ-1) |
  | `/presets/`, `/presets/tutorial/` with its six preset files, upgraded to version 2 | 6 |
  | `/debugger/` | 7 |
  | `/reference/` (every control, the settings) | 2, 5, 7 and 8, each control with its part |
  | `/modules/` | 9 |
  | `/faq/` | 9 (OQ-1) |
  | `/play/`, `/software/` and its articles | stay pages of the site (OQ-1) |
  | the file formats, today in `reference/StateFileFormat.md` | 10 |

  and chapter 9 covers cue cards, Solid State ROM modules, the TI-58C's
  constant memory and fast mode (`help.md`, fit table).
- [ ] **AC-10** — Given the reference chapter of Calc-U-59, then it
  describes, each in its current version: the preset format with the TI
  sections (LIB-6, C59-2), the magnetic card (C59-7), `ti58c.mem`
  (C59-3), the ASM overlay (C59-4), the settings file (LIB-9) and the
  keyboard map (LIB-5 AC-21).

### Calc-U-1600

- [ ] **AC-11** — Given the help of Calc-U-1600, then its product parts
  are written from its documents of today:

  | Today (`docs/`) | Goes to |
  |---|---|
  | `User-Guide.md` §1, §2 | 1 and 2 |
  | §3 Plotters | 5 |
  | §4 Loading a BASIC program, §6 Machine-code programs, §7 COM ports | 9 |
  | §5 Memory modules and floppy disks | 4; the folder as a drive: 9 |
  | §8 Presets | 6 |
  | §9 Appendix: settings; INSPECTOR; custom memory modules | 8; 7; 9 and 10 |
  | `Debugger.md` | 7 |
  | `Keyboard-Mapping.md`, `Memory-Card-Definition-Spec.md`, `Memory-Card-Definition-Format.md` | 10 |
  | `PC1600-Host-Drive.md`, `PC1500-Keyword-Modules.md` | 9 |

  (`docs/User-Guide.md:14-22`; `help.md`, fit table.)
- [ ] **AC-12** — Given the pictures of Calc-U-1600's help, then they
  are made as today, by the scenarios that drive the Qt6 frontend, one
  set for each chapter that has pictures, and they show the family's
  menus and control bar (`docs/developer/screenshots/README.md:1-120`).
- [ ] **AC-13** — Given the reference chapter of Calc-U-1600, then it
  describes, each in its current version: the preset format with the
  Sharp sections and the YAML of `debug:` and `bus-rom:` (LIB-6,
  C1600-4 AC-20), the module definition and the floppy disk file
  (LIB-7 AC-37), the settings file (LIB-9) and the keyboard map (LIB-5
  AC-29).

## Edge Cases
- **EC-1** — Given a product has nothing for the product part of a
  chapter that has both parts, then the chapter has its shared part
  only.
- **EC-2** — Given a help file links to something outside the help (the
  site, a source repository), then the build does not follow the link
  and does not fail on it.
- **EC-3** — Given a product part needs to say what differs by model in
  a chapter that has only a shared part (chapter 3), then it stands in
  chapter 9 and the shared part points there.

## Technical Requirements (optional)
- The help lives in the source repository: the shared chapters with the
  library, the product chapters with each product (ADR 0036).
- A concept that two products share is described once (`help.md` I-2).
- The help of a release describes that release (`help.md` I-1).
- There is no way to pull a product's text into a shared part (`help.md`
  HL-1).

## Open Questions
- [ ] **OQ-1** — The pages of calc-u.org/59 that are not a user guide:
  installing on each platform, the FAQ, the software pages with their
  articles, and the page with the web calculator. *Recommendation:*
  installing goes into chapter 2 as a product part, the FAQ into chapter
  9; the software pages, the articles and the page with the calculator
  stay pages of the site and are not in the asset.
- [ ] **OQ-2** — The table of contents of the asset (AC-6). `help.md`
  HL-3 asks for one and gives no form. *Recommendation:* one JSON file,
  `toc.json`, with the product, the version, and for each chapter its
  number, title and files in order, each marked shared or product.
- [ ] **OQ-3** — The check of AC-5. *Recommendation:* the build looks
  for the words under "Avoid" and lists each place as a warning, not a
  failure, because several of them are common words.
- [ ] **OQ-4** — The pictures of Calc-U-59's help. The site has its own
  today; nothing makes them from the product, and those that show the
  window show 2.3.0's bar and no menus. *Recommendation:* pictures that
  show what changed are taken anew, by hand for the first release; the
  others are moved as they are. Making them by scenarios, as
  Calc-U-1600 does, is a later step.
- [ ] **OQ-5** — "Help", "shared part", "product part", "help asset" and
  "help tool" for the glossary, as defined above.
- [ ] **OQ-6** — The help tool (AC-6, AC-7). The index says this feature
  holds no library code. *Recommendation:* the tool is a small program
  of the source repository's tools, run as `tools/help build <product>`
  and `tools/help check <product>`; it is the one piece of code of this
  feature.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Help is Markdown in the source repository: shared chapters once, product chapters per product; Calc-U-59's is converted once | ADR 0036 | 2026-11-10 |
| Shared chapters are fully generic; product examples live in product chapters; no include mechanism | `help.md` HL-1 (user) | 2026-11-10 |
| The ten-chapter structure, in menu and control-bar order | `help.md` HL-2 | 2026-11-10 |
| Each release attaches `<Product>-help.zip` with Markdown, pictures and a table of contents; the site renders it | `help.md` HL-3 | 2026-11-10 |
| Help uses only glossary terms | `help.md` I-3 | 2026-11-10 |
| Proposed in this spec, for the gate: the mappings of AC-9 and AC-11 from today's pages and documents to the chapters | The contract gives the chapters, not where today's text goes | 2026-11-10 |
| Proposed in this spec, for the gate: the checks of AC-7 | They make "the help matches the release" checkable | 2026-11-10 |

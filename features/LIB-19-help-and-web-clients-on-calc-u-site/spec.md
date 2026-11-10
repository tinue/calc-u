# LIB-19: Help and web clients on calc-u-site

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-15 (the Web frontends and how a page embeds one), LIB-16
  (the help asset).
- Contracts and decisions: `docs/contracts/help.md` (HL-3, I-1),
  `docs/contracts/frontend.md` (FE-7); ADR 0036; Briefing §3 (the site
  stays its own repository and keeps its design), §4.3.
- Facts: calc-u-site at `acd629c`, as `docs/inventory/calc-u-site.md`
  records it; every path below is in that repository.

This feature is carried out in the repository calc-u-site, not in the
source repository. "The site" is https://calc-u.org; a **pin** is the
record, kept in the site, of one release file by its address, its
version and its sha256.

What the three status columns mean for this feature:
- **Library:** the site can take the help asset of any product, by its
  pin, and show it in the site's design; shown with a help asset built
  for the tests.
- **Calc-U-59:** `/59/` shows the help of the pinned Calc-U-59 release
  and its Web frontend.
- **Calc-U-1600:** `/1600/` exists, with the help of the pinned
  Calc-U-1600 release and its Web frontend.

## User Stories
- As a user, I want to read the help of each product on calc-u.org, in
  one design, and try the product in the browser from there.
- As a user, I want the help I read to belong to a release I can
  install.
- As the maintainer, I want to publish a new release's help by changing
  a pin, without rewriting pages.

## Out of Scope
- Writing the help and building the asset (owner: LIB-16); the Web
  frontends themselves (LIB-15 and the products).
- The site's design, fonts and style (calc-u-site; Briefing §3).
- The pages of the site that are not help: the root page, the software
  pages and their articles (calc-u-site; LIB-16 OQ-1).
- The old domain, which only redirects (Briefing §4.3).

## Acceptance Criteria

### Library

- [ ] **AC-1** — Given a product's folder on the site, then it has a pin
  for the product's help asset and one for its Web frontend, each with
  the address of a release file, its version and its sha256; a pin never
  names "the latest release" or a preview (`help.md` HL-3; today for the
  Web frontend `59/web-client.json:2-5`).
- [ ] **AC-2** — Given a pinned help asset, then the site's build makes
  one page for each chapter of its table of contents (LIB-16 AC-6), in
  the site's design, with the chapters' order as the navigation, the
  Markdown rendered, its Mermaid diagrams drawn and its pictures shown;
  the pages are static and load nothing from another site (`help.md`
  HL-3; today's rule `CLAUDE.md:53-55`).
- [ ] **AC-3** — Given the site's checks, then they fail when: the
  asset in the site differs from its pin; the pages differ from what the
  build makes from the asset; a link inside the help leads nowhere; or
  the pinned help and the pinned Web frontend are of different versions
  of the product (`help.md` I-1; today's checks `.github/workflows/site.yml:33-51`).
- [ ] **AC-4** — Given a new release of a product, then publishing its
  help is: changing the two pins, running the build, and committing what
  it made; no page is written by hand for it.
- [ ] **AC-5** — Given a product's folder, then a page of it embeds the
  product's Web frontend as LIB-15 AC-15 and AC-16 say, from the pinned
  release's `dist/`, unchanged (`frontend.md` FE-7; today
  `59/.seo/fetch-web-client.js`).

### Calc-U-59

- [ ] **AC-6** — Given `/59/`, then its help pages are those AC-2 makes
  from the pinned Calc-U-59 help asset, in place of the pages written in
  the site today (`59/Pages.jsx`; ADR 0036).
- [ ] **AC-7** — Given an address of `/59/` that works today
  (`59/routes.js:24-56`), then it still leads to a page with that
  content, at the same address or by a redirect (OQ-1).
- [ ] **AC-8** — Given `/59/play/` and the installable app under
  `/59/calc/app/`, then they are the pinned Web frontend of Calc-U-59,
  as today.
- [ ] **AC-9** — Given the presets the site hosts under
  `59/software-files/`, then they are those of the pinned release, in
  version 2 (C59-2 AC-12), and the site's checks compare them with the
  release's.
- [ ] **AC-10** — Given the tests of `/59/` today (`59/tests/`, 29
  tests), then those about navigation, fonts and the calculator page
  still pass, with the addresses of AC-7.

### Calc-U-1600

- [ ] **AC-11** — Given `/1600/`, then it is no longer a placeholder: it
  has the help pages AC-2 makes from the pinned Calc-U-1600 help asset,
  a page with the Web frontend of the pinned release, and the app that
  can be installed from it (Briefing §4.3; LIB-15 AC-10, AC-24).
- [ ] **AC-12** — Given the root page of the site, the sitemap and the
  rules for crawlers, then they name `/1600/` as they name `/59/`
  (`robots.txt:10-16`).
- [ ] **AC-13** — Given `/59/` and `/1600/` are open in one browser,
  then what each Web frontend keeps is apart from the other's (LIB-15
  AC-7).
- [ ] **AC-14** — Given the checks and tests of the site, then `/1600/`
  has a job like that of `/59/`: the pins, the build, the links, and a
  test that the page with the Web frontend starts a machine.

## Edge Cases
- **EC-1** — Given a help asset whose table of contents names a file
  that is not in it, then the build fails and names the file.
- **EC-2** — Given a help file links to another product's help or to a
  page of the site outside the help, then the link is kept as it is, and
  the check of AC-3 follows it inside the site.
- **EC-3** — Given a release has a Web frontend and no help asset, then
  it cannot be pinned for a folder that shows help.
- **EC-4** — Given a preview of a coming release is shown on the site,
  then it is apart from the pinned release and is not what AC-1 to AC-5
  describe (today `59/beta/`).

## Technical Requirements (optional)
- The site keeps its own repository, its design and its branch model
  (Briefing §3; `CLAUDE.md:14-36`).
- The help on the site always matches a released version (`help.md`
  I-1); nothing of the help is edited in the site.

## Open Questions
- [ ] **OQ-1** — The addresses of `/59/` (AC-7). Today's pages do not
  map one to one onto the ten chapters (LIB-16 AC-9). *Recommendation:*
  the chapters get addresses of their own, and every address of today
  redirects to the chapter that holds its content, as the site already
  redirects older names.
- [ ] **OQ-2** — The address of the Web frontend under `/1600/`.
  *Recommendation:* `/1600/play/` and `/1600/calc/app/`, as under
  `/59/`.
- [ ] **OQ-3** — The content plan for `/1600/` that was approved before
  this project says that there is no page with a calculator. It is out
  of date, since 0.8.0 has a Web frontend. *Recommendation:* this spec
  replaces it.
- [ ] **OQ-4** — "Pin" for the glossary, as defined above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The site imports and renders the help of the release it pins, as it pins the Web frontend; it keeps its repository and design | ADR 0036; `help.md` HL-3; Briefing §3 | 2026-11-10 |
| `/1600/` is built: the help from Calc-U-1600's documents and its published Web frontend | Briefing §4.3 | 2026-11-10 |
| One embedding interface for the Web frontends | `frontend.md` FE-7 | 2026-11-10 |
| Proposed in this spec, for the gate: the help and the Web frontend that a folder pins are of one version (AC-3) | `help.md` I-1 | 2026-11-10 |
| Proposed in this spec, for the gate: `/1600/` gets a job of checks and tests like `/59/` (AC-14) | Today only `/59/` is checked | 2026-11-10 |

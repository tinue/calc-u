# The help site: calc-u.org

calc-u.org is served from its own repository, calc-u-site
(`tinue/calc-u-site`, locally `../calc-u-site`). That repository keeps its
own rules, design, branch model and skills; all site work is done there,
by them. The help itself is written only in the source repository and is
never edited in the site.

## What the site shows for each product

Each product has a **site folder**: `/59/` for Calc-U-59, `/1600/` for
Calc-U-1600. In each:

- **Help pages:** the pages of the product's help, made by the site's
  build from the pinned help asset, in the site's design. Each chapter
  has an address of its own; how the pages look and how they are reached
  is the site's choice.
- **`/<folder>/play/`:** a page that embeds the product's pinned Web
  frontend.
- **`/<folder>/calc/app/`:** the installable app made from the same Web
  frontend.
- **Hosted files** (Calc-U-59): the presets and the four ASM overlays
  under `59/software-files/`, taken from the source repository's
  `examples/` at the pinned release tag, in the current versions of
  their formats.
- **Site pages that are not help** (root page, software pages and
  articles, play page) stay the site's own.
- **Previews:** a build of a coming release may be shown next to the
  released one; a preview is never pinned, is kept
  apart from the released pages, and the checks below do not apply to it.

What the two Web frontends keep in the browser (settings, save folder)
stays apart when `/59/` and `/1600/` are open in one browser.

## How the site takes a release (specification)

**Pins.** Each site folder keeps three pins, never "the latest release"
and never a pre-release:

| Pin | Records | Example (Calc-U-59 3.0.0) |
|---|---|---|
| Help asset | address, version, sha256 | `https://github.com/tinue/calc-u/releases/download/calc-u-59-v3.0.0/Calc-U-59-help.zip` |
| Web frontend | address, version, sha256 | `…/calc-u-59-v3.0.0/Calc-U-59-web.zip` |
| Examples | the release tag in `tinue/calc-u` | `calc-u-59-v3.0.0` |

A release that has a Web frontend and no help asset cannot be pinned for
a folder that shows help.

**Build.** The site's build makes the help pages from the asset's
`toc.json` (chapters in order, their files, shared or product part): it
renders the Markdown, draws the Mermaid diagrams, shows the pictures and
resolves links and anchors. Links inside one product's help are relative
to the file, with heading anchors made by GitHub's rule for Markdown
headings (`library/help/anchor_cases.txt` lists cases to test against);
every other link is a full `https://` address and is kept as it is. The
pages are static and load nothing from another site (Mermaid and fonts
are served from calc-u.org). If `toc.json` names a file that is not in
the asset, the build fails and names the file. The Web frontend is
embedded from the release's `dist/` unchanged, through the embedding
interface. The hosted example files are taken from the source
repository at the examples pin.

**Checks.** The site's checks fail when:
- the asset in the site differs from its pin (address, sha256);
- the pages differ from what the build makes from the asset;
- a link inside the help leads nowhere (a link to another product's help
  or to a site page outside the help is followed inside the site);
- the help, the Web frontend and the examples pins are not all of one
  version of the product;
- (Calc-U-59) a hosted example file differs from the source repository
  at the pinned tag.

**Publishing a release.** Change the three pins, run the build, commit
what it made, on the branch the site's rules give that folder and version
(`<folder>/<version>`, merged into `main`, which publishes). No page is
written by hand for a release.

**Test asset.** The site's tests run the build and the checks on a small
help asset that the help tool built for that purpose and that is kept
with the site's tests.

Formats: the asset's layout and `toc.json` are specified in
`docs/formats/help.md`, the embedding interface of the Web frontends in
`docs/formats/web-embed.md`.

## What exists today

### In the source repository (tinue/calc-u)

- **Help tool** `tools/help check|build <product>` (`tools/README.md`),
  Python on the standard library, with its tests in
  `tools/helptool/tests/` on a fixture help tree. It checks the ten
  chapters and their parts, links, anchors and pictures, that shared
  parts name nothing of one product, that every settings key is
  described, and that no source path or migration step is named; words
  the glossary lists under "Avoid" are warnings.
- **Asset:** `<Product>-help.zip` with `toc.json` (product, version,
  chapters, files marked shared or not), `shared/`, `product/` with
  pictures and downloads; links rewritten to their places in the zip;
  two builds give the same bytes.
- **Chapters:** eight shared parts in `library/help/`; Calc-U-59's
  product parts in `products/calc-u-59/help/` (22 files, eight pictures,
  six tutorial presets in format 2 under `downloads/`); Calc-U-1600's in
  `products/calc-u-1600/help/` (20 files, pictures made by scenarios).
  Both `tools/help check` runs pass.
- **Builds and CI:** each product's tests build its asset; each product
  workflow has a job `help` (the local Linux CI has none: the help
  builds the same on the Mac); each `release.yaml` attaches the help zip.
- **Releases:** the pre-releases `calc-u-59-v3.0.0-pre` and
  `calc-u-1600-v0.9.0-pre` carry `Calc-U-59-help.zip` /
  `Calc-U-1600-help.zip` and the `-web.zip` files. The final releases
  `calc-u-59-v3.0.0` and `calc-u-1600-v0.9.0` are not published yet.
- **Embedding:** the Web frontends expose `CalcU59` and `CalcU1600` with
  the shared props `width`, `height`, `keyboard`, `layout`, `model`,
  `preset`, `workerUrl` (Calc-U-59 adds `settings`).

### In calc-u-site

- `main` is the published site; branches `<folder>/<version>`
  (`59/2.3.0`). The `site-59` skill's edit that points the preset
  grammar at `docs/formats/preset.md` of the source repository is
  committed on `59/2.3.0`, not merged into `main`.
- **`/59/`:** the complete Calc-U-59 help as hand-written React JSX
  (`59/Pages.jsx`, `routes.js`), prerendered to static HTML by
  `59/.seo/build.js`. The Web frontend is Calc-U-59 2.3.0's, pinned in
  `59/web-client.json` (address, sha256,
  version, commit), installed into `59/calc/` by
  `59/.seo/fetch-web-client.js`; `/59/play/` mounts it as
  `PlayCalculator`. `59/software-files/` holds 39 presets without a
  `format-version` line, 2 Markdown guides and the four `.asm` overlays,
  copied by hand from release 2.3.0; `59/tutorial-files/` the six
  tutorial presets in the same form. No preview is shown; `59/preview/`
  holds a design preview page.
- **`/1600/`:** one placeholder page with a link to GitHub.
- **Checks:** `.github/workflows/site.yml`, one job `site-59`:
  re-download the pinned Web frontend and diff `calc/`; rebuild and diff;
  `check.js` (about 1300 assertions); Playwright `59/tests/` (29 tests:
  5 fonts, 17 navigation, 7 play). Nothing for `/1600/`.
- **Crawlers:** `robots.txt` names only `/59/sitemap.xml` and blocks
  `/59/`'s app, frontend files, preview and beta.
- **Skills:** `site-59`, `site-59-design`; no skill and no design for
  `/1600/`. The site's own content plan for `/1600/` (no play page) does
  not apply; this document does.
- **Text to correct:** `README.md` describes a `gh-pages` copy serving
  another domain; `CLAUDE.md` calls `/1600/` a placeholder and points at
  other repositories than tinue/calc-u.

### Where the content of the `/59/` pages is in the help

| Address | Help file (Calc-U-59 product part) |
|---|---|
| `/`, `/what-is-a-ti-59/` | `01-introduction.md` |
| `/getting-started/` | `02-getting-started.md` |
| `/install/iphone-ipad/`, `/install/mac/`, `/install/windows-linux/`, `/install/android/` | `02-installing.md` |
| `/getting-started/printer/` | `05-printer.md` (cards: `04-magnetic-cards.md`) |
| `/presets/`, `/state-files/` | `06-presets.md` |
| `/presets/tutorial/` | `06-preset-tutorial.md` |
| `/debugger/` | `07-debugger.md` |
| `/reference/` | split: 02, 05, 07, `08-settings.md` |
| `/modules/` | `09-modules.md` |
| `/faq/` | `09-faq.md` |
| `/play/`, `/software/`, articles | stay site pages |

The deep links `/getting-started/install-*`, `/getting-started/presets/`,
`/getting-started/debugger/`, `/getting-started/state-files/` and the
`#hash` addresses (`LEGACY_HASHES` in `59/routes.js`) need redirects
too.

## To do

### `/1600/` (when Calc-U-1600 0.9.0 is released)
- [ ] Decide the design of `/1600/` with the site's skills, before any page.
- [ ] Write the site's renderer for a pinned help asset (shared by both
      folders) and its checks; build and keep the small test asset.
- [ ] Pin the help asset, Web frontend and release tag of 0.9.0; build
      the help pages.
- [ ] `/1600/play/` with `CalcU1600` and the installable app at
      `/1600/calc/app/`.
- [ ] Keep the Web frontend's stored data apart from `/59/`'s; test with
      both open in one browser.
- [ ] Sitemap for `/1600/`, named in `robots.txt`; block its app and
      frontend files as for `/59/`.
- [ ] A workflow job for `/1600/` like `site-59`: pins, build, links, and
      a start test (the play page's display shows what the model shows
      after a start).

### `/59/` (when Calc-U-59 3.0.0 is released; until then `/59/` stays as it is)
- [ ] Give each chapter an address; replace the JSX help pages with the
      pages built from the pinned 3.0.0 help asset.
- [ ] Redirect every address of the table above, the deep links and the
      `#hash` addresses to the chapter that holds its content; a site
      test walks the whole table.
- [ ] `/59/play/` and `/59/calc/app/` from the pinned 3.0.0 Web frontend
      of tinue/calc-u; change the embed once from `PlayCalculator` to
      `CalcU59` with the shared props.
- [ ] Take `59/software-files/` (presets and overlays) from `examples/`
      at the pinned tag, in the current formats, and check them against it;
      drop `tutorial-files/` (the tutorial presets ship in the asset).
- [ ] Rewrite the navigation tests for the chapters' pages and the
      redirects; the font and play tests pass unchanged.
- [ ] Make the release links of `59/Pages.jsx` (three found; look for a
      fourth) exact links to the release in tinue/calc-u.

### Site knowledge
- [ ] Site `CLAUDE.md` and `README.md`: remove the `gh-pages` text,
      describe `/1600/`, the pins, and tinue/calc-u as the source.
- [ ] Update `site-59` and `site-59-design` for the pinned help; merge
      the `site-59` edit of `59/2.3.0` into `main`; write a skill for
      `/1600/`.

### Help content (source repository)
- [ ] Calc-U-59: retake from the Swift app of the main checkout the four
      pictures with the 2.3.0 control bar (no module picker, PC-100C
      button or More: `app-screenshot.png`, `ipad-13-2752x2064.png`,
      `ipad-2752x2064-asm.png`, `iphone-duo-2852x2006.png`) and the two cue
      card pictures that show a magnetic card's side badges on a preset's
      plain card (`cuecard-key-grid.png`, `cuecard-single-label.png`); take
      the two missing ones (the macOS window with its menus; the iPhone's
      bar with the More menu open). Every picture with the calculator shows
      the new faceplate and cue cards then (also `iphone-debug.png`).
- [ ] Calc-U-1600: retake with `tools/make_screenshots.sh <chapter>` from a
      terminal with the screen-recording and Automation rights (iTerm2):
      "Tape:" on the bar (`01-app-overview.png`, `04-tape-recording.png`,
      `04-tape-loading.png`, `05-floppy-picker.png`, `05-slot-picker.png`),
      "Tape name:" (`04-tape-save.png`), "Card YAML" (`01-app-overview.png`,
      `09-debug-panel.png`), "Reset All" and no Resume
      (`02-machine-menu.png`, `02-rom-menu-pc1500.png`), "Tapes", "Examples
      folder", "Basic folder" (`09-settings.png`).
- [ ] Glossary warnings: the help tool lists the places in each
      product's help of words under "Avoid", most of them common words
      in another sense; narrow the glossary's lists or fix
      the text.
- [ ] The help of each product counts as finished when its final release
      carries the help asset (Calc-U-1600 0.9.0, Calc-U-59 3.0.0).
- [ ] Later: make Calc-U-59's pictures by scenarios, as Calc-U-1600 does.

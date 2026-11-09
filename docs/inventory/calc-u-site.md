# Inventory: calc-u-site

Pinned: `main` @ `acd629c` (2026-11-08, the published site; re-pinned
2026-11-09, see "Changes from `a383fbc` to `acd629c`"). Read on 2026-11-08
at branch `59/2.3.0` @ `a383fbca563005d87ee373cd3d6bd467ad14074a`
(2026-11-07); statements about `59/beta/` and the 2.2.0 client below
describe that read. At that time `main` (= `origin/main`) was
`7d70834a2fcdbb40ccc769b3fba43156cf5e8bac`; `59/2.3.0` was 2 commits ahead of
and 1 merge commit behind `main`, differing in 7 files under `59/` (page text
and `sitemap.xml` only). Working tree clean.
Repository: `/Users/me/Development/public/calc-u-site`, GitHub
`tinue/calc-u-site`.

Paths below are relative to the repository root and read at the pinned SHA,
unless marked *live* (checked over HTTP on 2026-11-08).

## Changes from `a383fbc` to `acd629c`

- `72ebd62` "Calc-U-59 2.3.0 is released: pin its web client, remove the
  2.3.0 preview", merged into `main` as `acd629c`.
- `59/web-client.json` pins
  `https://github.com/tinue/Calc-U-59/releases/download/v2.3.0/Calc-U-59-web.zip`,
  version 2.3.0, commit `915a2b5…`; `59/calc/` is that client (*live*
  `calc/MANIFEST.json` reports 2.3.0, checked 2026-11-09).
- `59/beta/` (60 files) and `59/beta-client.json` are removed; the "Try the
  2.3.0 preview" link on `/play/` is gone (was `59/Pages.jsx:2148`).
- Other `Pages.jsx` citations below are unchanged (they precede the
  removed lines).

## Summary
- Static site for https://calc-u.org, served by GitHub Pages straight from
  `main` ("Deploy from a branch"); `CNAME` is `calc-u.org`.
- Three areas: a root landing page, `/59/` (the complete Calc-U-59 help
  site, with a build step) and `/1600/` (a one-paragraph placeholder).
- `/59/` is authored as React JSX, compiled and prerendered to static HTML by
  `59/.seo/build.js`; CI only re-checks, it never builds or commits.
- The Calc-U-59 web client is imported unchanged from a Calc-U-59 release
  asset into `59/calc/` (pinned by sha256), plus a pre-release preview in
  `59/beta/`.
- At the pinned SHA and live, `59/calc/` is web client **2.2.0**, although
  Calc-U-59 2.3.0 was released on 2026-11-07; 2.3.0 is only the `-pre`
  preview in `59/beta/`.
- calcu59.ch answers with a 301 redirect to `https://calc-u.org/59…`
  from an nginx server, not from GitHub Pages and not from any repository read
  here.
- The root `README.md` is stale: it still describes a `gh-pages` copy serving
  calcu59.ch. Several files under `59/` carry stale paths and statements too.
- 49 downloadable program files (`.ti59`, `.ti58`, `.ti58c`, `.asm`), 2 `.md`
  files in `59/software-files/`, and 3
  Markdown/PDF article sets are hosted; none of the `.ti5x` files has a
  version line.

## Site structure and build

### Root (`/`)
- Files: `index.html`, `404.html`, `robots.txt`, `CNAME`, `style.css`,
  `.nojekyll`, `README.md`, `CLAUDE.md` (`CLAUDE.md:6-10`).
- `index.html:13-17`: "This site is under construction", links to `/59/`
  and `/1600/`.
- `404.html` and `robots.txt` serve all sites, because GitHub Pages and
  crawlers only look at the root (`CLAUDE.md:51-52`, `README.md:10-11`).
- `robots.txt:10-14` blocks `/59/calc/app/`, `/59/calc/index.html`,
  `/59/calc/$`, `/59/preview/`, `/59/beta/`; `robots.txt:16` names only
  `https://calc-u.org/59/sitemap.xml`.

### `/1600/`
- One file, `1600/index.html`. Content: "Emulator for the Sharp PC-1500,
  PC-1500A and PC-1600. Under construction." and a link to
  `github.com/tinue/Calc-U-1600` (`1600/index.html:12-13`).
- No build, no skill, no tests, no CI job (`.github/workflows/site.yml` has
  only the job `site-59`, `site.yml:17-18`).

### `/59/`
- Base path `"/59"` (`59/routes.js:82`); every internal URL in page text must
  start with `/59/`, enforced by `check.js` (`59/.seo/README.md:12-27`).
- Sources (hand-written): `Pages.jsx` (2246 lines, every page component),
  `components.jsx`, `site.jsx` (client router), `routes.js` (URL map),
  `styles.css`, `colors_and_type.css`, `assets/`, `fonts/`,
  `software-files/`, `tutorial-files/`, `articles/*.md`
  (`59/.seo/README.md:206-208`).
- Generated and committed: `build/` (compiled JSX), every route's
  `index.html`, `sitemap.xml` (`59/.seo/README.md:202-204`). `build.js`
  compiles every `.jsx` and prerenders each route with title, description,
  canonical, Open Graph and JSON-LD (`59/.seo/README.md:111-125`).
- Articles: `articles/<name>.md` rendered to `.html` with pandoc and to
  `.pdf` with `md2pdf`; neither tool is in CI, so outputs are committed
  (`59/.seo/README.md:210-222`).
- Routes (`59/routes.js:24-56`): `/`, `/what-is-a-ti-59/`, `/play/`,
  `/getting-started/`, `/install/iphone-ipad/`, `/install/mac/`,
  `/install/windows-linux/`, `/install/android/`, `/presets/`,
  `/presets/tutorial/`, `/debugger/`, `/reference/`, `/modules/`,
  `/software/`, `/faq/`, plus canonical-redirect routes for old names
  (`/state-files/` → `/presets/`, old `/getting-started/...` deep links).
  Old `#hash` URLs are rewritten client-side (`LEGACY_HASHES`,
  `59/routes.js:58-80`).
- Name history: "State files" was the old site name for presets
  (`59/routes.js:41-45`).
- Rules: nothing is fetched from a third party, no analytics
  (`CLAUDE.md:53-55`, `59/.seo/README.md:165-181`, reasoning in
  `59/fonts/README.md:7`).
- `lastmod` in `sitemap.xml` is deliberately the build date
  (`59/.seo/README.md:157-163`).

### Branch model (site-specific)
- `main` receives merges only; one branch per site folder and emulator
  version, `<site folder>/<emulator version>` (for example `59/2.2.0`), at
  most four branches (`CLAUDE.md:14-36`).
- Existing branches: `main`, `59/2.2.0`, `59/2.3.0` (local and origin).
- A `1600/…` branch changes only `1600/` (`CLAUDE.md:28-29`); none exists.

### Checks and tests
- `.github/workflows/site.yml`: on push to `main`, on pull requests and on
  dispatch (`site.yml:11-15`); one job on `ubuntu-24.04`, Node 24
  (`site.yml:18-28`):
  1. re-download the pinned web client and `git diff --exit-code -- calc`
     (`site.yml:33-36`);
  2. `node .seo/build.js` and diff the output, `sitemap.xml` excluded
     (`site.yml:40-43`);
  3. `node .seo/check.js` with jsdom (`site.yml:45-48`);
  4. `tests/run-tests.sh` (`site.yml:50-51`).
- `59/tests/`: Playwright, 3 spec files (`fonts.spec.js`, `nav.spec.js`,
  `play.spec.js`), 29 `test(` calls; `run-tests.sh` serves the repository
  root on port 8000 and runs Chromium (`59/tests/run-tests.sh:1-33`).
- Elapsed time of the `Site` workflow on `main` (GitHub Actions, `gh run
  list`): 59 s (2026-11-04), 42 s (2026-11-03), 35 s (2026-10-25).
- Number of `check.js` assertions: "400-odd" (`59/.seo/README.md:188`)
  versus "1100+" (`.claude/skills/site-59/SKILL.md:24`); the actual number
  was not counted (no run).

## Material per product

| Area | Calc-U-59 (`/59/`) | Calc-U-1600 (`/1600/`) |
|---|---|---|
| Pages | 15 indexable routes (above) | 1 placeholder page |
| Playable web client | `/59/play/` and `/59/calc/app/` (PWA), from `59/calc/` | none on calc-u.org |
| Downloads | 45 files in `software-files/`, 6 in `tutorial-files/`, 3 articles (md/html/pdf) | none |
| Screenshots | `59/assets/` (8 PNG), `calc/img/`, `beta/img/` | none |
| Design system | `59/README.md`, `colors_and_type.css`, skill `site-59-design` | none |
| Agent skills | `site-59`, `site-59-design` | none (planned `site-1600-design`, `site-1600`: memory file, see `agent-knowledge.md`) |

A content outline for `/1600/` exists outside the repository, in
`~/.claude/plans/rustling-napping-pelican.md` (2026-10-22, "approved" per
the calc-u-site memory file `site-1600-content-plan.md`). It says the site
will document 0.8.0 and has "No /play/: there is no web client"
(`rustling-napping-pelican.md`, section "Decisions"). Calc-U-1600 0.8.0
ships a web client (see below), so this statement is out of date.

## How the web clients are embedded and where they come from

### Calc-U-59 (embedded)
- Source: Calc-U-59 `Web/` (contract: Calc-U-59
  `reference/WebClient.md:29-33` at `v2.3.0`), built by Calc-U-59's
  `build.yml` job `web` and attached to each release as
  `Calc-U-59-web.zip` (`59/.seo/README.md:31-35`).
- `59/web-client.json:2-5` pins URL
  `https://github.com/tinue/Calc-U-59/releases/download/v2.2.0/Calc-U-59-web.zip`,
  its sha256, version `2.2.0` and commit `0f8f731…`.
- `59/.seo/fetch-web-client.js` installs it (`--pin <url>`, `--local
  <dist>`, `--beta <url>`) and replaces `calc/` as a whole; `check.js` fails
  if `calc/` differs from its `MANIFEST.json` or from the pin
  (`59/.seo/README.md:37-48`).
- Every page loads `calc/calc.css` and `calc/calc.js`; `build.js` evaluates
  `calc.js` so `/play/` prerenders with the real `PlayCalculator`
  (`59/.seo/README.md:50-57`, `59/.seo/build.js:131-136`, `:184`, `:275`,
  `:321`). `PlayPage` only sizes it: `layout="wide"` or `"portrait"`,
  `width`, `keyboard` (`59/Pages.jsx:2084-2091`).
- Beta: `59/beta/` is the `v2.3.0-pre` zip (`59/beta-client.json:2-5`,
  version 2.3.0, commit `5074f87…`); `/play/` links to `/59/beta/app/`
  ("Try the 2.3.0 preview", `59/Pages.jsx:2148`). Nothing re-downloads it,
  because promotion deletes the `-pre` tag (`59/.seo/README.md:59-86`). The
  beta and the released client share `localStorage` on the origin
  (`59/.seo/README.md:78-80`).
- `calc/MANIFEST.json` and the *live* `https://calc-u.org/59/calc/MANIFEST.json`
  both report version `2.2.0`.
- The skill forbids pinning `releases/latest/` or a `-pre` URL
  (`.claude/skills/site-59/SKILL.md:55-62`); the Calc-U-59 `release.yaml`
  comment names `releases/latest/download/Calc-U-59-web.zip` as a way to
  fetch it (Calc-U-59 `release.yaml:16-19` at `v2.3.0`).

### Calc-U-1600 (not embedded)
- Not present in calc-u-site.
- Calc-U-1600 0.8.0 builds `Calc-U-1600-web.zip` (Calc-U-1600
  `release.yaml:36-37`, `docs/developer/Web-Client.md:59-60` at `v0.8.0`).
  Its `dist/` contract: location-independent, embedded as
  `React.createElement(CalcU1600, props)` with props `width`, `height`,
  `keyboard`, `model`, `preset`, `workerUrl`, styles scoped to
  `.calcu-client` (`docs/developer/Web-Client.md:62-89`).
- Calc-U-59's component is `PlayCalculator` with `layout`, `width`,
  `keyboard` (above); Calc-U-1600's is `CalcU1600`. Both scope styles to
  `.calcu-client`.
- *Live*: the 0.8.0 web client is published as a preview at
  `https://tinue.github.io/1600/` (named in Calc-U-1600 `README.md:32-50`
  and `CHANGELOG.md` 0.8.0 entry at `v0.8.0`). It is served from the private
  repository `tinue/tinue.github.io` (pushed 2026-11-07). Its page calls
  itself a "Temporary harness". Its `calc/MANIFEST.json` reports version
  0.8.0, commit `246f099` (on `dev-0.8.0`; tree `88f45ff`, not the
  `v0.8.0` tree `bf9f277`).

## Content duplicated or near-duplicated

Within calc-u-site:
- `59/calc/presets/*` (10 files) are byte-identical copies of the same names
  in `59/software-files/` (same sizes and headers). `59/beta/presets/*` holds
  the same 10 names again.
- Fonts: `59/fonts/archivo-400.woff2` and `archivo-700.woff2` also ship in
  `59/calc/fonts/` and `59/beta/fonts/`. React: `59/vendor/` and
  `59/calc/vendor/`, `59/beta/vendor/`.
- `59/calc/` and `59/beta/` share most files; their `MANIFEST.json` files
  list identical hashes for ROM text files, images and presets, and
  different ones for `calc.js`, `calc.css`, `app/*` and the WASM core.

Between calc-u-site and Calc-U-59 (`v2.3.0`):
- `59/software-files/` is hand-copied from Calc-U-59 `examples/`; nothing
  syncs them (`.claude/skills/site-59/SKILL.md:76-95`). A read-only
  comparison against `examples/` at `v2.3.0` (excluding `debug/` and
  `screentest/`): every hosted file is byte-identical to its counterpart;
  `examples/assembly/README.md` and `examples/calendar/CALENDAR_PROGRAMS.md`
  are not hosted. `tutorial-files/` exist only on the site.

Between the Calc-U-59 site and Calc-U-1600 user documentation (topics only;
Calc-U-1600 has no site yet):

| Topic | Calc-U-59 site | Calc-U-1600 at `b4ea8c8` (0.8.1) |
|---|---|---|
| Presets: what a preset is, smallest preset, step-by-step tutorial | `/presets/`, `/presets/tutorial/` (`Pages.jsx:673`, `:970`) | `docs/User-Guide.md:674-910` (§8 "Presets", 8.1 "What a preset is", 8.2 "The smallest preset") |
| Debugger | `/debugger/` (`Pages.jsx:1375`) | `docs/Debugger.md` |
| Install on Windows / Linux: unsigned installer, `chmod +x` the AppImage | `/install/windows-linux/` (`Pages.jsx:577-597`) | `release.yaml:19-23` notes suffix (same wording as Calc-U-59 `release.yaml:31-35`) |
| Dropping files onto the window | `Pages.jsx:614` | `docs/User-Guide.md:167` ("Dropping files") |
| Installing the web client as an app (PWA) | `/install/android/` (`Pages.jsx:625-652`) | `README.md:32-38` |
| Keyboard | `/reference/`, `/faq/` | `docs/Keyboard-Mapping.md` |
| Settings | `/reference/` | `docs/User-Guide.md:914` (§9.1) |
| Removable items | `/modules/` (Solid State Software modules) | `docs/User-Guide.md:374-526` (§5 "Memory modules and floppy disks") |

## Redirect from calcu59.ch
- *Live*: `https://calcu59.ch` → `301` → `https://calc-u.org/59`;
  `https://www.calcu59.ch/<path>` → `301` → `https://calcu59.ch/<path>` →
  `301` → `https://calc-u.org/59/<path>` (path preserved). Server header
  `nginx/1.18.0`; `calcu59.ch` and `www.calcu59.ch` resolve to
  `185.3.235.176`. calc-u.org resolves to GitHub Pages addresses
  (`185.199.108-111.153`) and answers with `server: GitHub.com`.
- No configuration for this redirect exists in calc-u-site or in Calc-U-59
  at `v2.3.0` (`git grep calcu59.ch` finds only `README.md:14` here and two
  historic `CHANGELOG.md` lines in Calc-U-59, `CHANGELOG.md:76`, `:153`).
- Calc-U-59 has no `gh-pages` branch, locally or on `origin`
  (`git ls-remote --heads origin`).

## Stale README and other stale statements

| Where | What it says | What is true |
|---|---|---|
| `README.md:13-17` | The Calc-U-59 site "started as a copy of the `gh-pages` branch of the Calc-U-59 repository, which still serves calcu59.ch until launch; until that branch is retired, a page change made in one place has to be made in the other." | No `gh-pages` branch exists; calcu59.ch is an nginx 301 redirect to calc-u.org/59; `/59/` is the only source (Briefing §4.3). |
| `README.md:8` | `/1600/` — "Calc-U-1600 (Sharp PC-1500 / PC-1500A / PC-1600)" | It is a placeholder (`1600/index.html:12`); `CLAUDE.md:10` says so. |
| `59/.seo/README.md:7` | `cd docs` before building | The site lives in `59/`; the skill says `cd 59` (`.claude/skills/site-59/SKILL.md:22`). |
| `59/build/README.md:1-2` | "edit the .jsx sources in docs/" | The sources are in `59/`. |
| `59/README.md:3` | User guide for an app "available on Mac, iPhone, and iPad" | Calc-U-59 also ships for Windows, Linux and the web (Briefing §4.2; `/install/windows-linux/`, `/play/`). |
| `59/README.md:26` | GitHub repository "not reachable from the sandbox; visuals are reconstructed from the screenshot" | Written for a sandboxed design tool; the repository is local and public. |
| `59/README.md:82` | Self-hosting fonts "recommended for the `.ch` domain" | The site is served from calc-u.org. |
| `59/.seo/README.md:188` vs `.claude/skills/site-59/SKILL.md:24` | 400-odd vs 1100+ `check.js` assertions | Both stale: 1302 checks at `acd629c` (run 2026-11-09). |

## Downloadable files hosted on the site

Format tags: `.ti59`/`.ti58`/`.ti58c` are Calc-U-59 state files (own format,
`reference/StateFileFormat.md` in Calc-U-59); `.asm` are TI-59 assembler
sources for the Calc-U-59 debugger (own format); `.md`/`.html`/`.pdf` are
documents. Every `.ti5x` file below starts with `#` comments followed by
directives such as `MODEL:`, `SOLID-STATE-MODULE:`, `PRINTER:`,
`PARTITION:`, `CUECARD:`, `PROGRAM:`, `KEYSTROKES:`. None contains a
`VERSION:` or `FORMAT:` line (`grep -iE '^\s*(format|version)\s*:'` over
`software-files/`, `tutorial-files/`, `calc/presets/` finds nothing).
Whether that makes them a legacy format is agent A's finding.

`59/software-files/` (45 files):
- `.ti59` (36): `calendar-01-Weinberger`, `calendar-02-cargile`,
  `calendar-03-vanderburgh`, `calendar-04-skillman`,
  `calendar-05-vanderburgh`, `calendar-06-cargile`, `calendar-07-calidas`,
  `calendar-08-acosta`, `calendar-09-dispatch`, `characters-fastmode`,
  `characters`, `diag`, `erdkundetest-2`, `erdkundetest`,
  `fast-grafik-3d-plot`, `FastMode_Zero4`, `fastmode-sst`, `fastmode`,
  `firmware`, `Frequency-Transformation`, `globe-print`,
  `Integer-Base-Conversion`, `keycodes`, `life`, `pdcm-card-tricks`,
  `pdcm-make-cards`, `picard`, `printer-interrupt`, `printer-quirks`,
  `ram_test_full_fast`, `ram_test`, `stars-and-stripes`, `sum`,
  `switzerland`, `texas-print`, `trace-quirk`.
- `.ti58` (2): `diag`, `fastmode`.
- `.ti58c` (1): `repartition`.
- `.asm` (4), in `software-files/assembly/`: `Decoder`, `DPT`,
  `simple_count`, `stopwatch`. `Decoder.asm` opens with "This program
  *doesn't work* on the emulator" (first line of the file).
- `.md` (2): `PDCM.md`, `PDCM-Emulator.md`.

`59/tutorial-files/` (6, `.ti59`): `tutorial-1-setup`,
`tutorial-2-partition`, `tutorial-3-cuecard`, `tutorial-4-program`,
`tutorial-5-registers`, `tutorial`. Not in Calc-U-59 at `v2.3.0`.

`59/articles/` (3 × md/html/pdf): `fast-mode`, `printer-interrupt`,
`ti-58c-changes`.

Bundled with the embedded web client (`59/calc/presets/`, also
`59/beta/presets/`): `characters.ti59`, `diag.ti58`, `diag.ti59`,
`fastmode-sst.ti59`, `globe-print.ti59`, `picard.ti59`,
`printer-interrupt.ti59`, `ram_test.ti59`, `sum.ti59`, `switzerland.ti59`,
and `manifest.json` (file name and title per preset). ROM text files
(`calc/wasm/roms/*.txt`, 25 files) and `cuecards.txt` ship inside the web
client.

## Open questions
- ~~calcu59.ch redirect~~: answered 2026-11-09. It runs at a hosting
  provider for as long as the domain is paid for; the new project never
  uses or mentions the old domain (Briefing §4.3).
- ~~Is pinning `59/calc/` to 2.3.0 pending?~~ Done in `acd629c`.
- ~~Is `https://tinue.github.io/1600/` part of Calc-U-1600's release
  material?~~ Answered 2026-11-09: the developer keeps it, but it is not part
  of this project; the web client eventually goes to calc-u.org/1600.
- Is the `/1600/` content outline in `~/.claude/plans/rustling-napping-pelican.md`
  still valid, given its "no web client" assumption?
- ~~Actual number of `check.js` assertions~~: 1302 at `acd629c` (run
  2026-11-09).

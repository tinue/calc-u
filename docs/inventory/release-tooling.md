# Inventory: release tooling

Pinned:
- ReleaseIt: `/Users/me/Development/public/ReleaseIt`, branch `main` @
  `02c41c6918c4977af4115445d1c75c29d891d9cd` (2026-11-07), working tree
  clean.
- Calc-U-59: `v2.3.0` @ `231792c`.
- Calc-U-1600: `v0.8.1`, assumed released; until it is tagged, `dev-0.8.1`
  @ `b4ea8c8` (tree `947b850`). Read at `v0.8.0` @ `d976d3f` (tree
  `bf9f277`); between the two, `release.yaml`, `build.yml` and
  `tools/fetch_sharpdx.sh` are unchanged, `Qt6/CMakeLists.txt`,
  `tools/refresh_sharpdx.sh` and `libsharpdx.a` changed (below).

Read on 2026-11-08. ReleaseIt paths are relative to its root; product paths
are prefixed `59:` or `1600:` and read at the pinned tag. Run times come from
`gh run view` (read-only).

## Summary
- ReleaseIt is a Rust CLI (`release`, plus `renew-certs`) installed on macOS
  only (`README.md:17-23`). One invocation handles one project: the git
  repository root, one `release.yaml` there, one version, one CHANGELOG,
  one `main`, one dev branch, one set of tags `vX.Y.Z` / `vX.Y.Z-pre`, one
  GitHub repository (whatever `gh` resolves from `origin`).
- Cycle: `release` (pre-release under a disposable `-pre` tag, repeatable) →
  `release --rc` (dates the CHANGELOG entry) → `release --release`
  (squash-merge the dev branch onto `main`, tag, retarget the pre-release in
  place) → `release --next` (cut `dev-<next>`, bump) (`README.md:32-83`).
- Calc-U-59: Swift macOS app built, signed and notarised locally (Xcode,
  `xcode-dmg`); Qt6 Windows/Linux and the web client from a dispatched
  `build.yml`; iOS via `release ios` (local) and an `appstore` orphan
  branch worktree.
- Calc-U-1600: nothing built locally; one dispatched `build.yml` builds
  every platform, including the macOS DMG signed and notarised in CI with
  repository secrets managed by `renew-certs`.
- Calc-U-1600 pulls SharpDataExchange (SDE) `libsharpdx` per platform: a
  committed macOS and WASM library, and a release download (`v0.3.5`) for
  Linux/Windows in CI.
- `examples/Calc-U-59.yaml` and `examples/Calc-U-1600.yaml` in ReleaseIt
  are older than the products' actual `release.yaml` files (both lack the
  web client; the Calc-U-59 one also lacks the CI build).

## How ReleaseIt works

### Entry and configuration
- `release` `cd`s to `-C <dir>` if given, then to `git rev-parse
  --show-toplevel`, "which both tools treat as the project root"
  (`src/sh.rs:114-123`).
- It loads exactly one file, `<root>/release.yaml` (`src/config.rs:7`,
  `:306-311`), reads one version from it (`src/main.rs:73-76`), and builds
  one context with `tag = "v{version}"`, `pre_tag = "v{version}-pre"`
  (`src/ctx.rs:39-48`).
- `release.yaml` keys (`src/config.rs:9-33`; reference `README.md:85-132`):
  `name` (required; release titles and DMG names), `main_branch` (default
  `main`, `:71-73`), `version` (`preset: xcode | cargo | cmake | regex`,
  `file`, `regex`), `changelog` (default `CHANGELOG.md`, wip marker
  `work in progress`, `insert_next: top | after-unreleased`, `:109-118`),
  `next` (`bump` default minor, `branch` default `dev-{version}`,
  `after_bump`, `extra_files`, `:142-151`), `checks`, `release` (`title`
  default `"{name} {version}"`, `notes_suffix`, `commit_trailer`,
  `extra_assets`, `post_release`, `:180-190`), `build`, `appstore`, `certs`.
- Version presets: `xcode` reads the first `MARKETING_VERSION = …;`,
  `cargo` `^version = "…"`, `cmake` `^set(APP_VERSION "…")`
  (`src/version.rs:18-23`). `--next` with `xcode` bumps every occurrence
  equal to the current version and resets `CURRENT_PROJECT_VERSION` to 0
  (`README.md:134-138`).

### Modes
- `release` (`src/modes/pre.rs`): refuses on `main` (`:20-21`); stashes
  unstaged changes, requires a clean index, an upstream `origin/<branch>`,
  sync with it and fast-forwardability into `main` (`:22-40`,
  `src/git.rs:82-126`); requires a non-empty `## [X.Y.Z]` CHANGELOG
  section (`:28-35`); runs `checks` (`:43-45`); checks that every
  `github-dispatch` workflow exists on the default branch (`:49-60`);
  writes App Store notes if `appstore:` is set (`:62-64`); then
  `git tag -f vX.Y.Z-pre` and `git push -f origin vX.Y.Z-pre` (`:83-84`),
  runs the build steps and creates or updates the GitHub pre-release
  (`:90-98`, `src/gh.rs:32-59`), then `post_release` (`:102`).
- `release --rc`: as above, after replacing the open header with today's
  date, committing "Set X.Y.Z release date to …" and pushing
  (`src/modes/pre.rs:105-118`). `--reopen` undoes the date
  (`README.md:59-62`).
- `release --release` (`src/modes/promote.rs`): requires a dated CHANGELOG
  entry (`:24-31`) and a pre-release with assets (`:33-41`); checks out
  `main`, fast-forwards it from `origin` (`src/git.rs:24-30`),
  `git merge --squash <dev branch>`, commits "Release vX.Y.Z" plus the
  CHANGELOG section, pushes `main` (`:61-68`); tags `vX.Y.Z` there and
  pushes it (`:70-72`); `gh release edit vX.Y.Z-pre --tag vX.Y.Z
  --prerelease=false --latest` (`:78-85`); deletes the `-pre` tag
  (`:87-89`); verifies the asset count is unchanged (`:96-106`).
- `release --next` (`src/modes/next.rs`): from `main`, bumps the version
  per `next.bump` (or `--version`), creates `next.branch` with
  `{version}` replaced (`:27-40`), inserts `## [<next>] - <wip>`
  (`:50-52`), commits "Bump version to <next>" and pushes it with
  `-u origin` (`:54-60`).
- `release --build-only`: checks and local build steps only
  (`src/modes/mod.rs:20-38`).
- Known quirk: a binary that embeds `git describe` still says
  `vX.Y.Z-pre` after promotion (`README.md:82-83`,
  `src/modes/promote.rs:7-8`).

### Build steps (`src/build/`)
- Kinds: `commands` (local), `xcode-dmg` (local), `github-dispatch` (CI),
  `github-tag-push` (CI) (`README.md:147-178`).
- Order: CI steps are started first, local steps run meanwhile, then CI is
  awaited (`src/build/mod.rs:55-91`). Nothing is published unless every
  step succeeds (`src/build/mod.rs:1-4`).
- `github-dispatch`: `gh workflow run <workflow> --ref vX.Y.Z-pre`
  (`src/build/ci.rs:30-34`); the run is found by workflow, event, commit
  SHA and start time (`src/gh.rs:69-96`), watched at a 30 s interval
  (`src/gh.rs:98-109`), its artifacts downloaded and each mapped
  `artifact`/`file` glob → asset name `as`, exactly one match each
  (`src/build/ci.rs:38-71`).
- `xcode-dmg`: archive for `platform=macOS,arch=arm64` with `ARCHS=arm64`,
  Developer ID export, `create-dmg`, `codesign`, and in a real pre-release
  `notarytool` + staple (`src/build/xcode_dmg.rs:1-67`). The DMG is named
  `{app}-{version}.dmg` (`src/build/xcode_dmg.rs:15`). Signing identity:
  `$SIGN_IDENTITY` or the first "Developer ID Application" in the keychain;
  notarisation profile `notarytool` (`README.md:185-192`).
- `extra_assets`: directories zipped from the working tree
  (`src/build/mod.rs:93-100`).
- Asset names must be unique across steps (`src/build/mod.rs:102-108`).

### App Store (`src/appstore/`)
- With `appstore:` set, every pre-release converts the CHANGELOG section to
  App Store text, asks for confirmation and commits it as `whatsnew.txt` in
  the `appstore` worktree, commit message `whatsnew: vX.Y.Z`
  (`README.md:197-210`, `src/appstore/whatsnew.rs:99-130`).
- CHANGELOG lines can carry `<!-- appstore: skip -->` or
  `<!-- appstore: only -->`; `notes_forbidden` words stop the release
  (`README.md:214-228`).
- `release ios`: archive for iOS, export, validate, upload to TestFlight;
  requires a GitHub release `vX.Y.Z-pre` or `vX.Y.Z` for the same version
  (`src/appstore/ios.rs:145-153`, `README.md:230-233`).
- `release ios --release`: checks metadata and screenshots (age limit
  `screenshot_max_age_days`, default 60), verifies the processed build has
  the local `CURRENT_PROJECT_VERSION`, creates or finds the App Store
  version, uploads description, keywords, URLs, What's New and
  screenshots, attaches the build; submitting for review is manual
  (`README.md:234-238`). Credentials: App Store Connect API key in
  `~/.appstoreconnect/private_keys/`, `APP_STORE_API_KEY_ID`,
  `APP_STORE_API_ISSUER_ID` (`README.md:241-244`).
- Worktree layout: `appstore/metadata/*.txt`, `screenshots/iphone-NN-*.png`
  (1320×2868), `ipad-NN-*.png` (2752×2064), `iphoneduo-NN-*.png`,
  `whatsnew.txt` (`README.md:204-210`).

### `renew-certs`
- Reads `certs:` from the same `release.yaml`; rotates the Developer ID
  certificates used by CI: `guide`, `status`, `apply` (sets the two `.p12`
  secrets and the password secret, records `…_SHA1`/`…_EXPIRES` variables),
  `backup` (1Password) (`README.md:246-304`).
- Secrets and variables are set with `gh secret set` / `gh variable set`
  without `--repo`, i.e. on the repository of the current checkout
  (`src/renew_certs/github.rs:11`, `:19`, `:33`). "Run it once in every
  project with a `certs:` section" (`README.md:285-287`).

### Tests
- `cargo test`: unit tests, `tests/lifecycle.rs` (temporary git
  repositories, fake `gh`), `tests/renew_certs.rs` (`README.md:312-318`).
  ReleaseIt's own `release.yaml` runs clippy and `cargo test` as `checks`
  (`release.yaml:13-15`). Fixtures include
  `tests/fixtures/CHANGELOG-Calc-U-59.md` and `CHANGELOG-Calc-U-1600.md`.

## Single-product assumptions

Each item is a fact about the code; where several products would share one
repository, the item names what is fixed today.

| # | Assumption | Source |
|---|---|---|
| S1 | The project root is the git repository root; there is no way to point `release` at a subdirectory as project root (`-C` only changes where `git rev-parse --show-toplevel` starts). | `src/sh.rs:114-123` |
| S2 | Exactly one config file, `release.yaml`, at that root. | `src/config.rs:7`, `:306-311`; `README.md:3-5` |
| S3 | One `name`, one `version` source per invocation. | `src/config.rs:12-16`, `src/main.rs:73-76` |
| S4 | Release tag is `v{version}`, pre-release tag `v{version}-pre`; no product prefix; not configurable. | `src/ctx.rs:39-48` |
| S5 | One CHANGELOG file with one `## [X.Y.Z]` section per version. | `src/config.rs:109-118`, `src/changelog.rs:1`, `:8-10` |
| S6 | One main branch (default `main`); `release` and `release ios` refuse to run on it. | `src/config.rs:14-15`, `:71-73`; `src/git.rs:32-36` |
| S7 | Dev branches from one template, default `dev-{version}`; `--next` cuts it from `main`. | `src/config.rs:142-151`, `src/modes/next.rs:31-40` |
| S8 | `--release` squash-merges the whole dev branch into `main` as one commit "Release vX.Y.Z". | `src/modes/promote.rs:61-68` |
| S9 | The remote is named `origin` (hard-coded in fetch, push, `ls-remote`, upstream checks). | `src/git.rs:27`, `:87-91`, `:116-118`, `:133`; `src/modes/pre.rs:84`; `src/modes/promote.rs:68`, `:72`, `:88` |
| S10 | One GitHub repository: every `gh release`, `gh workflow`, `gh run`, `gh secret`, `gh variable` call omits `--repo`. | `src/gh.rs:24-59`, `:72-113`; `src/build/ci.rs:32`; `src/renew_certs/github.rs:11-33` |
| S11 | Promotion marks the release `--latest`. | `src/modes/promote.rs:83` |
| S12 | A dispatched workflow is located by file name, event and commit; two dispatches of the same workflow for the same commit are told apart only by start time. | `src/gh.rs:69-96` |
| S13 | One `appstore` worktree path and branch per repository; `whatsnew.txt` is one file. | `src/config.rs:266` (`AppStoreCfg`), `src/appstore/ios.rs:69-76`, `src/appstore/whatsnew.rs:115-130` |
| S14 | `certs:` secret names default to fixed names per repository. | `src/config.rs:57-65` |
| S15 | Release title template has only `{name}` and `{version}`. | `src/config.rs:163-166`, `:335-338` |
| S16 | Version bump: one version file; the `xcode` preset rewrites every `MARKETING_VERSION` equal to the current one in that file. | `src/version.rs:18-23`; `README.md:134-138` |

## Calc-U-59 at `v2.3.0`

### `release.yaml` (`59:release.yaml`)
- `name: Calc-U-59`; version preset `xcode`, file
  `Calc-U-59.xcodeproj/project.pbxproj` (`:21-25`). The project has
  `MARKETING_VERSION = 2.3.0` and, for another target, `= 1.0`
  (`59:Calc-U-59.xcodeproj/project.pbxproj:503`, `:592`).
- `post_release: bin/install-release.sh` (`:27-29`): installs
  `build/release/Calc-U-59.app` into `/Applications`, quitting and
  restarting a running copy (`59:bin/install-release.sh:1-25`).
- `notes_suffix`: Linux `chmod +x`, Windows installer unsigned (`:30-35`).
- Build steps (`:37-49`): `xcode-dmg` (project `Calc-U-59.xcodeproj`,
  scheme `Calc-U-59`, `bin/ExportOptions.plist`), then `github-dispatch`
  of `build.yml` with five artifacts:
  `Calc-U-59-linux-x86_64.AppImage`, `Calc-U-59-linux-aarch64.AppImage`,
  `Calc-U-59-windows-x86_64.exe`, `Calc-U-59-windows-arm64.exe`,
  `Calc-U-59-web.zip`.
- `appstore:` (`:51-57`): worktree `appstore`, bundle id
  `ch.erzberger.calcu59`, `bin/AppStoreExportOptions.plist`,
  `notes_forbidden: [Windows, Linux, Qt, AppImage, Android]`.
- No `checks:` and no `certs:` section.
- Comment: the web zip "has a fixed asset name, so the website's build can
  fetch it by tag, or once promoted (--release) as
  releases/latest/download/Calc-U-59-web.zip" (`:16-19`).

### `.github/workflows/build.yml` (`59:.github/workflows/build.yml`)
- Trigger: `workflow_dispatch` only (`:14-15`). No macOS job on purpose
  (`:4-7`). ROMs are committed under `roms/` (`:9-10`).
- Jobs:
  - `core-tests`, `ubuntu-24.04`: root CMake with
    `-DCALCU59_BUILD_QT=OFF`, `ctest` (`:18-32`).
  - `web`, `ubuntu-24.04`: emsdk `6.0.12`, Node 24, `Web/build.sh`,
    Playwright Chromium and WebKit, `Web/run-tests.sh`, zip
    `Calc-U-59-web-<version>.zip`, artifact `Calc-U-59-web` (`:34-69`).
  - `linux`, matrix x86_64 (`ubuntu-24.04`) / aarch64
    (`ubuntu-24.04-arm`): apt Qt6, `cmake -S Qt6`, linuxdeploy +
    Qt plugin (downloaded from `continuous` releases), AppImage, artifact
    `Calc-U-59-linux-<arch>` (`:71-138`).
  - `windows`, matrix x86_64 (`windows-latest`) / arm64
    (`windows-11-arm`): Qt `6.8.2` via `install-qt-action`, MSVC, Ninja,
    `windeployqt`, Inno Setup installer, artifact
    `Calc-U-59-windows-<arch>` (`:140-198`).
- No signing in CI (no secrets referenced).
- Elapsed time, run `42388503215` (2026-11-07, `v2.3.0-pre`): whole run
  3 min 53 s; `core-tests` 55 s, `linux` x86_64 2 min 25 s, aarch64
  3 min 16 s, `windows` x86_64 2 min 40 s, arm64 3 min 49 s, `web`
  3 min 35 s. Earlier runs: 4 min 4 s, 4 min 2 s, 3 min 54 s.

### What is built where

| Artifact | Built | Signed / notarised |
|---|---|---|
| `Calc-U-59-2.3.0.dmg` (Swift, arm64) | locally, Xcode (`xcode-dmg`) | Developer ID, notarised and stapled locally |
| iOS/iPadOS app | locally, `release ios` | App Store signing; TestFlight / App Store Connect |
| Linux AppImages (x86_64, aarch64) | GitHub Actions | none |
| Windows installers (x86_64, arm64) | GitHub Actions | unsigned |
| `Calc-U-59-web.zip` | GitHub Actions | none |

- Assets of the `v2.3.0` GitHub release: `Calc-U-59-2.3.0.dmg`,
  `Calc-U-59-linux-aarch64.AppImage`, `Calc-U-59-linux-x86_64.AppImage`,
  `Calc-U-59-web.zip`, `Calc-U-59-windows-arm64.exe`,
  `Calc-U-59-windows-x86_64.exe` (`gh release view v2.3.0`). The DMG name
  carries the version; the other names do not.

### App Store pipeline
- Metadata lives on the orphan branch `appstore` (local and `origin`),
  checked out as worktree `appstore/`. At `origin/appstore` (latest commit
  `ffd1cd2` "whatsnew: v2.3.0"): `metadata/description.txt`,
  `keywords.txt`, `marketing_url.txt`, `privacy_url.txt`, `subtitle.txt`,
  `support_url.txt`; `screenshots/ipad-01-assembly.png`,
  `ipad-02-calc-debugger.png`, `ipad-03-texas.png`,
  `iphone-01-plain-calculator.png`, `iphone-02-cpu-inspector.png`,
  `iphone-03-calc-debugger.png`, `iphone-04-diagnostics.png`,
  `iphone-05-settings.png`, `iphoneduo-03-calc-debugger.png`;
  `whatsnew.txt`.
- `subtitle.txt` exists on the branch but is not in the ReleaseIt README's
  list (`README.md:206`).
- Screenshot tooling in the product: `59:bin/run-screenshots`,
  `59:bin/simulators`, `59:bin/simulators.yaml` (contents not read here;
  agent A covers release material).

## Calc-U-1600 at `v0.8.1`

### `release.yaml` (`1600:release.yaml`)
- `name: Calc-U-1600`; version preset `cmake`, file `Qt6/CMakeLists.txt`
  (`:9-13`), which has `set(APP_VERSION "0.8.1")`
  (`1600:Qt6/CMakeLists.txt:39`).
- `notes_suffix`: Linux `chmod +x` (`:19-23`).
- `extra_assets`: `examples/` zipped as `Calc-U-1600-examples.zip`
  (`:24-25`).
- One build step, `github-dispatch` of `build.yml`, six artifacts
  (`:27-37`): `Calc-U-1600-linux-x86_64.AppImage`,
  `Calc-U-1600-linux-aarch64.AppImage`, `Calc-U-1600-mac-aarch64.dmg`,
  `Calc-U-1600-windows-x86_64.exe`, `Calc-U-1600-windows-arm64.exe`,
  `Calc-U-1600-web.zip`.
- `certs: team_id: 7Q3QN9AV9J` (`:39-42`). No `checks:`, no `appstore:`,
  no `post_release`.

### `.github/workflows/build.yml` (`1600:.github/workflows/build.yml`)
- Trigger: `workflow_dispatch` only; "The eventual plan is to trigger on
  pre-release tag creation instead" (`:34-37`).
- Every job runs `tools/fetch_roms.sh` first: ROMs are not committed and
  are downloaded from `tinue/PC-1500-ROM` and `tinue/PC-1600-ROM`
  (`:3-7`; `1600:tools/fetch_roms.sh:1-31`).
- Jobs:
  - `core-tests`, `ubuntu-24.04`: the only job that configures the root
    CMake (CLI harnesses, probes, `CoreTests`); needs apt Qt6 because the
    root adds `Qt6/`; fetches ROMs and sharpdx; `ctest` (`:39-83`).
  - `web`, `ubuntu-24.04`: emsdk `6.0.13`, Node 24, ROMs,
    `tools/build_wasm_check.sh`, `Web/build.sh`, Playwright Chromium and
    WebKit, zip `Calc-U-1600-web-<version>.zip` (`:86-145`).
  - `linux-x86_64` (`ubuntu-24.04`) and `linux-aarch64`
    (`ubuntu-24.04-arm`): apt Qt6, sharpdx fetch, AppImage via linuxdeploy
    (`:146-394`).
  - `mac-aarch64`, `macos-15`: Homebrew Qt (`brew install qt cmake
    ninja`, `:404`), `macdeployqt`, Developer ID signing with hardened
    runtime from repository secrets `APPLE_CERT_APPLICATION_P12_BASE64`,
    `APPLE_CERT_P12_PASSWORD`, notarisation with `APPLE_API_KEY_ID`,
    `APPLE_API_ISSUER_ID`, `APPLE_API_KEY_P8_BASE64`, staple with retry,
    `hdiutil` DMG, which is signed, notarised and stapled again
    (`:396-535`). Bundle id `ch.erzberger.CalcU1600Qt`
    (`1600:.github/macos-signing.md:12-14`). The installer certificate
    secret is set but unused (`1600:.github/macos-signing.md:16-19`).
  - `windows-x86_64` (`windows-latest`) and `windows-arm64`
    (`windows-11-arm`): Qt `6.8.2`, `windeployqt`, Inno Setup; the arm64
    job sets `SHARPDX_PLATFORM: windows-aarch64` (`:537-684`, `:649`).
- Elapsed time, run `42369839986` (2026-11-07, `v0.8.0-pre`): whole run
  7 min 46 s; `core-tests` 6 min 40 s, `linux-x86_64` 5 min 34 s,
  `linux-aarch64` 7 min 40 s, `mac-aarch64` 6 min 13 s, `windows-x86_64`
  6 min 4 s, `windows-arm64` 6 min 29 s, `web` 2 min 48 s. Earlier runs on
  `dev-0.8.0`: 7 min 46 s, 7 min 7 s, and one failure after 13 min 7 s.

### SDE artifact pull
- `libsharpdx` is vendored under `Core/Basic/vendor/sharpdx/`.
  Committed at `b4ea8c8`: `libsharpdx.a` (macOS, release build, 22 259 280
  bytes; 11 475 944 at `v0.8.0`),
  `libsharpdx-wasm.a` (4 400 388 bytes), `sharpdx.h`, `README.md`
  (`git ls-tree -l`).
- Linux and Windows CI jobs run `tools/fetch_sharpdx.sh`, which downloads
  `sharpdx-<platform>.<ext>` from
  `https://github.com/tinue/SharpDataExchange/releases/download/<tag>/`
  with `curl` (`1600:tools/fetch_sharpdx.sh:94-101`); default tag
  `v0.3.5`, overridable with `SHARPDX_TAG` (`:17-18`, `:52`); refuses to run
  on macOS, which uses the committed binary (`:78-80`). The fetched files
  are not committed (`:12-14`).
- The macOS binary is refreshed from a local SDE checkout with
  `tools/refresh_sharpdx.sh`, which since 0.8.1 refuses objects built for a
  newer macOS than `MACOSX_DEPLOYMENT_TARGET` (`check_minos`); the WASM one with
  `tools/build_sharpdx_wasm.sh` (`1600:tools/fetch_sharpdx.sh:8-11`;
  `1600:.github/workflows/build.yml:90-93`).
- It is a release-asset download, not a GitHub Actions artifact pull.

### What is built where

| Artifact | Built | Signed / notarised |
|---|---|---|
| `Calc-U-1600-mac-aarch64.dmg` (Qt6) | GitHub Actions (`macos-15`) | Developer ID, notarised and stapled in CI |
| Linux AppImages (x86_64, aarch64) | GitHub Actions | none |
| Windows installers (x86_64, arm64) | GitHub Actions | unsigned |
| `Calc-U-1600-web.zip` | GitHub Actions | none |
| `Calc-U-1600-examples.zip` | locally, by ReleaseIt (`extra_assets`, zip of the working tree) | none |

- Assets of the `v0.8.0` GitHub release: `Calc-U-1600-examples.zip`,
  `Calc-U-1600-linux-aarch64.AppImage`, `Calc-U-1600-linux-x86_64.AppImage`,
  `Calc-U-1600-mac-aarch64.dmg`, `Calc-U-1600-web.zip`,
  `Calc-U-1600-windows-arm64.exe`, `Calc-U-1600-windows-x86_64.exe`.

## Differences between the two products' release setups

| Aspect | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Version source | Xcode project (`xcode` preset) | `Qt6/CMakeLists.txt` (`cmake` preset) |
| macOS app | Swift, built and notarised locally | Qt6, built and notarised in CI |
| macOS DMG asset name | `Calc-U-59-<version>.dmg` | `Calc-U-1600-mac-aarch64.dmg` |
| iOS | `release ios`, `appstore` branch | none |
| CI signing secrets | none | 6 secrets, `certs:` with `renew-certs` |
| ROMs | committed (`roms/`) | downloaded per job (`tools/fetch_roms.sh`) |
| emsdk | 6.0.12 (`setup-emsdk@v16`) | 6.0.13 (`setup-emsdk@v14`) |
| `actions/setup-node` | `@v7` | `@v4` |
| CI `core-tests` without Qt | yes (`-DCALCU59_BUILD_QT=OFF`) | no (root CMake adds `Qt6/`) |
| Extra assets | none | `Calc-U-1600-examples.zip` |
| `post_release` | `bin/install-release.sh` | none |

## ReleaseIt examples vs product configurations
- `examples/Calc-U-59.yaml` lacks the `github-dispatch` step, `release:`
  (`post_release`, `notes_suffix`) and `notes_forbidden`; its comment says
  "Once the Qt6 Windows/Linux GitHub workflow exists, add it here"
  (`diff` against `59:release.yaml`).
- `examples/Calc-U-1600.yaml` lacks the `Calc-U-1600-web` artifact line;
  otherwise it equals `1600:release.yaml` (`diff`).

## Open questions
- Which GitHub repository is the release location after the move
  (Briefing §6, phase 3)? ReleaseIt resolves one repository from `origin`
  (S10).
- Is `-C <subdir>` meant to work for a product inside a monorepo? Today it
  still resolves to the repository root (S1).
- Is the Calc-U-1600 plan to trigger `build.yml` on tag push
  (`1600:.github/workflows/build.yml:35-36`) still current?
- `bin/run-screenshots` and `bin/simulators` in Calc-U-59 were not read
  here; their role in the App Store pipeline is left to agent A.

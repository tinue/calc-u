# Development and releases

This applies in full once Calc-U-59 3.0.0 and Calc-U-1600 0.9.0 are out;
until then `docs/phase-f.md` comes first. Short feature branches apply
already.

How a product's next version is developed and released from this
repository (ReleaseIt with `branching: trunk`; its README, "Several
products in one repository"). The example is
Calc-U-59 3.1.0 after 3.0.0; Calc-U-1600 works the same with
`-C products/calc-u-1600` and its tags `calc-u-1600-v…`.

- `main` is the only long-lived branch. There are no `dev-x.y.z` branches
  and no squash: a release only tags a commit on `main`.
- Only finished work is merged into `main`: a pre-release is always cut
  from it, and there is no dev branch to hold half-done work.
- Both products build from the same `main`. A library change made for one
  product ships in the other with its next release, so every product's
  tests stay green on `main`.

### A new version

1. **Bump the version**, once, on `main` (pushed, clean index):
   `release -C products/calc-u-59 --next`. It cuts no branch: it sets the
   version in the Xcode project to 3.1.0, adds
   `## [3.1.0] - work in progress` to the product's `CHANGELOG.md` and
   commits "Bump Calc-U-59 version to 3.1.0" on `main`.
2. **One short branch per change**, not one per release:
   `git switch -c <topic> main`, commit the change with its line under
   `## [3.1.0]` in the CHANGELOG, then
   `git switch main && git merge --no-ff <topic> && git branch -d <topic>`.
   Repeat until 3.1.0 is complete. Work on the other product merges into
   `main` in the meantime.
3. **Pre-release**, as often as needed, on `main`:
   `release -C products/calc-u-59`. It sets or moves the tag
   `calc-u-59-v3.1.0-pre` to the current `main`, builds the Mac app
   locally and dispatches `.github/workflows/calc-u-59.yml` for Windows,
   Linux, the web client and the help.
4. **Release candidate**: `release -C products/calc-u-59 --rc` dates the
   CHANGELOG entry and commits it on `main`.
5. **Release**: `release -C products/calc-u-59 --release` puts the tag
   `calc-u-59-v3.1.0` on the commit of the `-pre` tag, even if `main` has
   moved on since (for example by work on the other product), and turns
   the pre-release into the release, never marked "Latest".
6. Start the next version with step 1 (`--next`, 3.2.0).

### What a build shows

The About and the version line of the Settings show
`Version <version>[-<build number>] (<build ID>)`, in every frontend of
both products. The build number is the Swift app's
(`CURRENT_PROJECT_VERSION`; 3.0.0-0); Qt and the web have none. The build
ID comes from `tools/build-id.cmake`, at each build:

| Build | Build ID |
|---|---|
| Release candidate, and so the release | `a015f34`: the commit |
| Pre-release, and a local build before the release | `pre-a015f34`: the product's CHANGELOG entry is still "work in progress" |
| A build with changes not committed | `pre-a015f34-dirty` |

`--rc` dates the entry before it builds, so the build that becomes the
release has no `pre-`; `--release` builds nothing.

### A patch of a released version

A fix for 3.0.0 while 3.1.0 is in progress goes on a maintenance branch
cut from the release tag:

```
git switch -c calc-u-59/3.0.x calc-u-59-v3.0.0
```

There, `release -C products/calc-u-59 --next --version 3.0.1` bumps the
version (the default bump is minor), the fix is committed with its line
under `## [3.0.1]`, and the release runs there with the same `release`
commands (steps 3 to 5). The fix then also goes into `main`, by a merge or a
cherry-pick.

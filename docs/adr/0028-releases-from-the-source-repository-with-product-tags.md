# Releases come from the source repository, tagged per product

Status: accepted (2026-11-09)

Every release is a GitHub Release in the source repository, tinue/calc-u,
on a tag with the product's prefix (`calc-u-59-v2.4.0`,
`calc-u-1600-v1.0.0`) and with only that product's assets and notes; no
shared repository version exists. This is the usual pattern for
independently versioned products in one repository (release-please in
monorepo mode, changesets, Tauri, the Azure and AWS SDKs). ReleaseIt still
dispatches the product's workflow and waits for it (ADR 0004); there is
no build on tag push. No release is marked "Latest"
(`make_latest=false` / `gh release create --latest=false`), so GitHub's
"latest" never points at the wrong product; calc-u-site links each
product's downloads to its current release by exact tag, updated when the
site is re-pinned, as it already does for the web client. Neither app has
an auto-updater that could pick up the other product's release.

## Considered options

- Release repositories (the old product repositories kept for releases,
  fed by CI with a token): keeps today's URLs, but splits source and
  releases across repositories.
- A rolling `<product>-latest` tag per product: stable URLs, but CI must
  re-point and re-upload it on every release.

## Consequences

- ReleaseIt needs prefixed tags, `--latest=false` and per-product release
  titles (its single-product assumptions S1-S16, `release-tooling.md`).
- tinue/calc-u must be public before its first release; until development
  starts, hotfixes come from the old repositories (Briefing §3).
- The site's four links to `tinue/Calc-U-59/releases/latest` change to
  exact release links.

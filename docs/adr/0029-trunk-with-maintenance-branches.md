# Trunk with maintenance branches

Status: accepted (2026-11-09)

`main` is the only long-lived branch. Work happens on short branches
merged into `main`, history is not squashed, and a product release tags
`main` with its release tag (ADR 0028). A patch for an older version of one
product gets a maintenance branch from that release tag, for example
`calc-u-59/2.4.x`. This replaces today's `dev-x.y.z` branches whose
history is squashed into `main` at release, which no longer works with
two products releasing from one `main`.

## Considered options

- `dev-` branches per product, squashed into `main` at release: closest to
  today, but the squashes of two products would interleave on `main`.

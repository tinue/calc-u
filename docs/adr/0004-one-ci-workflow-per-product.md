# One CI workflow per product

Status: accepted (2026-11-09)

Each product has its own GitHub Actions workflow (for example
`calc-u-59.yml`, `calc-u-1600.yml`), dispatched by ReleaseIt for that
product only, as today. Each workflow also runs the library's tests. A
release of one product therefore never waits for, or fails because of, the
other product's jobs, which keeps releases independent. A change to the
library alone is checked by dispatching both workflows by hand (or a small
library workflow).

## Considered options

- One workflow with a `product` input: less duplication, but ReleaseIt and
  every job would need the parameter, and the products' job sets differ
  (Swift is built locally, Calc-U-1600 has a macOS job).

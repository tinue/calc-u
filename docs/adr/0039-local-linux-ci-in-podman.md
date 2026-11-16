# The CI's Linux jobs run locally in Podman before CI

Status: accepted (2026-11-16)

The Linux jobs of both workflows (`library`, `core-tests`, and the Qt build
of `linux-*`) also run locally, in a Podman container on Ubuntu 24.04 with
GCC 13 as on the runners (`ci/linux/run.sh`; `docs/test-groups.md`, "Local
Linux CI"). They run before CI is dispatched, natively on arm64. CI stays
the gate: x86_64, Windows, macOS signing and the AppImages run only there.

The local checks build with Apple clang, so a GCC-only error showed up only
in CI, 10 to 13 minutes after a dispatch; nearly all of the failed CI runs
of 2026-11-16 failed in a Linux build or test step (e.g. GCC 13's internal
compiler error fixed in `5bbd6f2`, which the container reproduces in a
second). A warm local run takes about 1.5 minutes, most of it the tests.

## Considered options

- Apple `container`: as fast, but younger and without the Docker-compatible
  tooling Podman has; Podman was installed already.
- `act` (runs the workflows themselves): fragile on Apple silicon with the
  large runner images, and the workflows' actions (emsdk, Qt) don't map.
- x86_64 under Rosetta as well: slower, and GCC's front-end errors are the
  same on arm64; the rare architecture differences are left to CI.
- The AppImage steps locally: none of the failures were there, and the
  steps would be copied out of the workflows.

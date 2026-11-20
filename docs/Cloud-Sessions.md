# Claude Code cloud sessions

How this repository is set up for Claude Code cloud sessions
(claude.ai/code, the Claude app's Code tab, `claude --cloud`). A cloud
session runs on a fresh Ubuntu 24.04 x86_64 VM that clones the repository
from GitHub. It needs the same things as the `core-tests` jobs of
`.github/workflows/calc-u-59.yml` and `.github/workflows/calc-u-1600.yml`.

## Two parts

| What | Where it lives | When it runs |
| :--- | :--- | :--- |
| Python 3, zlib and Qt6 apt packages | The cloud environment's **Setup script** (claude.ai/code → environment settings) | Once, then cached for ~7 days |
| ROMs (both products), Linux libsharpdx (Calc-U-1600) | `products/<product>/roms/fetch_roms.sh` and `products/calc-u-1600/tools/fetch_sharpdx.sh`, run by hand | At the start of a session that builds |

### Environment setup script

Paste this into the environment's **Setup script** field. It runs as root.

```bash
#!/bin/bash
apt-get update
apt-get install -y \
  qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev libgl1-mesa-dev \
  zlib1g-dev python3
```

Python 3 runs the help checks and the products' scripts among the tests
(Calc-U-59's `core-tests` installs it), zlib is Calc-U-1600's test
suite's (`CalcU1600CoreTests`); the Qt6 packages are those Calc-U-1600's
Linux jobs install for its Qt6 app. The VM already includes GCC, Clang,
cmake, ninja and git. Network
access **Trusted** (the default) covers the Ubuntu mirrors,
raw.githubusercontent.com (ROMs) and GitHub release downloads (sharpdx),
so no custom allowed domains are needed.

## Building and testing in a session

First fetch what the repository does not hold, from the repository root:

```bash
products/calc-u-59/roms/fetch_roms.sh
products/calc-u-1600/roms/fetch_roms.sh
products/calc-u-1600/tools/fetch_sharpdx.sh
```

Then build and test as the `core-tests` jobs do, both products in one
tree:

```bash
cmake -S . -B build/tests -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCALCU_FRONTEND_QT=OFF
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

No display, so the GUI apps (`-DCALCU_FRONTEND_QT=ON`, Qt needed) can't be
driven interactively. Use the harness `calcu --product c59` or
`calcu --product c1600` (the library's app session in emulated time; the
products' `CLAUDE.md`) for anything that needs to run an emulator.

## Limitations

- `products/calc-u-1600/core/Basic/vendor/sharpdx/sharpdx.h` may show as
  modified after the sharpdx fetch. Don't commit that change.
- Your user-level `~/.claude/CLAUDE.md` and auto-memory stay on your
  machine and don't reach cloud sessions. Only what's committed here does.

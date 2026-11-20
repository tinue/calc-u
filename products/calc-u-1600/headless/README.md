# headless/

Scratch space for the output of `calcu --product c1600` runs and other ad
hoc work: screenshots and traces (`--output-folder headless`), WAV files of
`audio` and `c1600 cassette record`, captures of `c1600 serial … capture`,
memory images (`peek … file=`), machine states (`state-save`), format-utility test images, disassembly
scratch, and similar. The harness itself is the target `calcu` of a root
CMake tree (`products/calc-u-1600/CLAUDE.md`, section Harness).

Everything in here except this file is ignored by version control: it's
all regenerable (rerun the command) or disposable. Nothing durable should
live here — a reusable example preset belongs in `examples/` (user-facing)
or `dev/` (debug/regression presets, hardware checks), a written-up
investigation belongs in `docs/`, and a permanent finding belongs in this
project's memory.

When running the harness or an ad hoc check, write its scratch output here
rather than the repo root.
